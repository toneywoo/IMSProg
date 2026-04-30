/*
 * Copyright (C) 2023 - 2025 Mikhail Medvedev <e-ink-reader@yandex.ru>
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "dialogsfdp.h"
#include "ui_dialogsfdp.h"
#include <QValidator>
#include <QRegExp>
#include "unistd.h"
#include "memory"
#include <QDebug>
#include <QRegularExpression>
#include "hexutility.h"
DialogSFDP::DialogSFDP(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogSFDP)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    setLineEditFilter();
    setRegStatus(1, true);
    numOfRegisters = 3; // 3-not reading, 2 - three registers, 1 - two registers, 0 - one register
    programmerType = 0;
}

DialogSFDP::~DialogSFDP()
{
    delete ui;
}

void DialogSFDP::legendPrint(QString basic, QString extended, QString manufacture)
{
QString l0 ="", l1="", l2="", l3="", h0="", h1="", h2="", h3="", h4="";
l0.append(QString(tr("Legend:")));
l1.append(QString(tr(" - Basic area")));
l2.append(QString(tr(" - Extended area")));
l3.append(QString(tr(" - Manufacture area")));
h0.append(QString("</span>"));
h1.append(QString("<html><head/><body><p>" + l0 + "</p><p>"));
h2.append(QString("<br><span style=\" background:#f77;\">"));
h3.append(QString("<br><span style=\" background:#7f7;\">"));
h4.append(QString("</p></body></html>"));
ui->label_9->setText(h1 + basic + h0 + l1 + h2 + extended + h0 + l2 + h3 + manufacture + h0 + l3 + h4);
}

void DialogSFDP::on_pushButton_clicked()
{
    int stCH341 = 0;
    uint64_t sfdpSize = 0;
    uint32_t sfdpBlockSize = 0;
    bool sfdpSupport = false;
    unsigned char i, imax, twoAreaAddress=0xff, manufAreaAddress=0xff, twoAreaLen = 0xff, manAreaLen = 0xff;
    uint8_t jedecMan=0xff, idSize;
    std::shared_ptr<uint8_t[]> sfdpBuf(new uint8_t[256]);
    QString regData = "", VCCmin = "", VCCmax = "", speeds = "Single", addrTxt="";
    int retval = 0;
    stCH341 = ProgDeviceInit( programmerType, 0, 1 );
    ui->lineEdit_vcc_max->setText("");
    ui->lineEdit_vcc_min->setText("");
    ui->lineEdit_block->setText("");
    ui->lineEdit_size->setText("");
    ui->lineEdit_speeds->setText("");
    ui->lineEdit_otp->setText("");
    legendPrint("**", "**", "**");
    if (stCH341 == 0)
    {
        //Reading JEDEC ID
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x9f, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),3,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading JEDEC ID!"));
           return;
        }
        jedecMan = sfdpBuf[0];
        ui->lineEdit_jedec0->setText(bytePrint(sfdpBuf[0]));
        ui->lineEdit_jedec1->setText(bytePrint(sfdpBuf[1]));
        ui->lineEdit_jedec2->setText(bytePrint(sfdpBuf[2]));


        // Reading SFDP. Transfer to ch341 0x5a
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x5a, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),256,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading register!"));
           return;
        }

        //Analyse-parsing SFDP
        if ((sfdpBuf[0] == 0x53) && (sfdpBuf[1] == 0x46) && (sfdpBuf[2] == 0x44) && (sfdpBuf[3] == 0x50))
        {
           ui->lineEdit_sfdp->setText("Yes");
           sfdpSupport = true;
        }
        else
        {
           ui->lineEdit_sfdp->setText("No");
           sfdpSupport = false;
           ui->label->setText("");
           ui->label_10->setText("");
        }
        if (sfdpSupport)
        {
           if ((sfdpBuf[0x80] == 0x53) && (sfdpBuf[0x81] == 0x46) && (sfdpBuf[0x82] == 0x44) && (sfdpBuf[0x83] == 0x50)) imax = 127;
           else imax = 254;
           twoAreaAddress = sfdpBuf[0x0c];
           twoAreaLen = sfdpBuf[0x0b] * 4;
           if (jedecMan == sfdpBuf[0x10])
           {
               manufAreaAddress = sfdpBuf[0x14];
               manAreaLen = sfdpBuf[0x13] * 4;
           }
           else
           {
               manufAreaAddress = sfdpBuf[0x1c];
               manAreaLen = sfdpBuf[0x1b] * 4;
           }
           if (manufAreaAddress != 0xff)
           {
              if ((sfdpBuf[manufAreaAddress] != 0xff) && (sfdpBuf[manufAreaAddress + 1] != 0xff))
              {
                  VCCmax = bytePrint(sfdpBuf[manufAreaAddress + 1]) + bytePrint(sfdpBuf[manufAreaAddress]);
                  VCCmin = bytePrint(sfdpBuf[manufAreaAddress + 3]) + bytePrint(sfdpBuf[manufAreaAddress + 2]);
                  VCCmax.insert(1, ".");
                  VCCmin.insert(1, ".");
                  ui->lineEdit_vcc_max->setText(VCCmax);
                  ui->lineEdit_vcc_min->setText(VCCmin);
              }
              if (sfdpBuf[manufAreaAddress + 9] != 0xff)
              {
                 if ((sfdpBuf[manufAreaAddress + 9] & 0x08) != 0) ui->lineEdit_otp->setText("Yes");
                 if ((sfdpBuf[manufAreaAddress + 9] & 0x08) == 0) ui->lineEdit_otp->setText("No");
              }
              else ui->lineEdit_otp->setText("");
           }
           else
           {
               ui->lineEdit_vcc_max->setText("");
               ui->lineEdit_vcc_min->setText("");
           }
           sfdpSize =( sfdpBuf[twoAreaAddress + 4] + sfdpBuf [twoAreaAddress +5] * 256 + sfdpBuf[twoAreaAddress +6] * 256 * 256 + sfdpBuf[twoAreaAddress + 7] * 256 * 256 * 256 + 1) /8 /1024 ;
           ui->lineEdit_size->setText(QString::number(sfdpSize) + " K");
           if (sfdpBuf[twoAreaAddress + 0x20] != 0xff)
           {
               sfdpBlockSize = (1 << sfdpBuf[twoAreaAddress + 0x20]) / 1024;
               ui->lineEdit_block->setText(QString::number(sfdpBlockSize) + " K");
           }
           else
           {
               ui->lineEdit_block->setText("");
           }
           if (sfdpBuf[twoAreaAddress + 0x0f] == 0xbb) speeds = speeds + "/Dual";
           if (sfdpBuf[twoAreaAddress + 0x09] == 0xeb) speeds = speeds + "/Quad";
           ui->lineEdit_speeds->setText(speeds);
           legendPrint("00", bytePrint(twoAreaAddress), bytePrint(manufAreaAddress));
           //HEXDUMP
           regData = tr("<html><head/><body><p> Hex SFDP register data:\n");
           addrTxt = tr("<html><head/><body><p>Addr:<br>");
           for (i=0; i<232;i=i+16)
           {
               addrTxt = addrTxt + "0" + bytePrint(i) + "><br>";
           }
           addrTxt = addrTxt + "0F0></p></body></html>";
           ui->label_10->setText(addrTxt);
           for (i=0;i<=imax;i++)
           {
              if (i % 16 == 0) regData = regData +  "<br> ";
              if (i == 0x0c) regData = regData + "<span style=\" background:#f77;\">";
              if (i == 0x0d) regData = regData + "</span>";

              if ((i == 0x14) && (jedecMan == sfdpBuf[0x10]) && (sfdpBuf[0x06] !=0)) regData = regData + "<span style=\" background:#7f7;\">";
              if ((i == 0x15) && (jedecMan == sfdpBuf[0x10]) && (sfdpBuf[0x06] !=0)) regData = regData + "</span>";
              if ((i == 0x1c) && (jedecMan != sfdpBuf[0x10]) && (sfdpBuf[0x06] !=0)) regData = regData + "<span style=\" background:#7f7;\">";
              if ((i == 0x1d) && (jedecMan != sfdpBuf[0x10]) && (sfdpBuf[0x06] !=0)) regData = regData + "</span>";

              if (i == twoAreaAddress) regData = regData + "<span style=\" background:#f77;\">";
              if (i == twoAreaAddress + twoAreaLen) regData = regData + "</span>";
              if (i == manufAreaAddress) regData = regData + "<span style=\" background:#7f7;\">";
              if (i == manufAreaAddress + manAreaLen) regData = regData + "</span>";
              regData = regData + bytePrint(sfdpBuf[i]) + " ";
           }
           regData = regData + "</p></body></html>";
           ui->label->setText(regData);

        }
        //READING STATUS REGISTER 0
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x05, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),2,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        usleep(1);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading register!"));
           return;
        }
        printRegData(0, sfdpBuf[0]);
        //READING STATUS REGISTER 1
        numOfRegisters = 0;
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x35, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),2,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        usleep(1);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading register!"));
           return;
        }
        if (sfdpBuf[0] != 0xff)
        {
            numOfRegisters++;
            setRegStatus(1, true);
            printRegData(1, sfdpBuf[0]);
        }
        else setRegStatus(1, false);

        //READING STATUS REGISTER 2
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x15, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),2,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        usleep(1);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading register!"));
           return;
        }
        if (sfdpBuf[0] != 0xff)
        {
            numOfRegisters++;
            setRegStatus(2, true);
            printRegData(2,sfdpBuf[0]);
        }
        else setRegStatus(2, false);

        //Reading Unique ID
        SPI_CONTROLLER_Chip_Select_Low(programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x4b, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        SPI_CONTROLLER_Write_One_Byte(0x00, programmerType);
        retval = SPI_CONTROLLER_Read_NByte(sfdpBuf.get(),16,SPI_CONTROLLER_SPEED_SINGLE, programmerType);
        SPI_CONTROLLER_Chip_Select_High(programmerType);
        if (retval)
        {
           QMessageBox::about(this, tr("Error"), tr("Error reading unique ID!"));
           return;
        }
        if ((sfdpBuf[0x00] == sfdpBuf[0x08]) && (sfdpBuf[0x01] == sfdpBuf[0x09]) && (sfdpBuf[0x02] == sfdpBuf[0x0a]) && (sfdpBuf[0x03] == sfdpBuf[0x0b])) idSize = 8;
        else idSize = 16;
        regData = "";
        for (i=0; i<idSize; i++)
        {
            regData = regData + bytePrint(sfdpBuf[i]);
        }
        ui->lineEdit_chipid->setText(regData);

        //Close the CH341a device
        ProgDeviceClose( programmerType );
    }
    else QMessageBox::about(this, tr("Error"), tr("Programmer ") + programmerName + tr(" is not connected!"));

}

void DialogSFDP::on_pushButton_2_clicked()
{
    DialogSFDP::close();
}

void DialogSFDP::on_pushButton_3_clicked()
{
   if (numOfRegisters < 3)
   {
    int stCH341 = 0;
    stCH341 = ProgDeviceInit( programmerType, 0, 1 );
    if (stCH341 == 0)
    {
       //scanning data from the form
       uint8_t r[3], k, i;
       int j;
       r[0] = 0;
       r[1] = 0;
       r[2] = 0;
       for (k = 0; k < 3; k++)
       {
           i = 128;
           for (j = 7; j >= 0; j--)
           {
               QString currRegName = "lineEdit_sr" + QString::number(k) + QString::number(j);
                       for (QLineEdit* edit : findChildren<QLineEdit*>())
                       {
                           if (edit->objectName() == currRegName)
                           {
                               if (QString::compare(edit->text(), "0", Qt::CaseInsensitive)) r[k] = r[k] + i;
                               i = i / 2;
                           }
                       }
           }
       }

       //Writing status registers 0,1 for Winbond

       SPI_CONTROLLER_Chip_Select_Low(programmerType);
       SPI_CONTROLLER_Write_One_Byte(0x06, programmerType);
       SPI_CONTROLLER_Chip_Select_High(programmerType);
       usleep(1);

       SPI_CONTROLLER_Chip_Select_Low(programmerType);
       SPI_CONTROLLER_Write_One_Byte(0x50, programmerType);
       SPI_CONTROLLER_Chip_Select_High(programmerType);
       usleep(1);

       SPI_CONTROLLER_Chip_Select_Low(programmerType);
       SPI_CONTROLLER_Write_One_Byte(0x01, programmerType);
       SPI_CONTROLLER_Write_One_Byte(r[0], programmerType);
       if (numOfRegisters > 0) SPI_CONTROLLER_Write_One_Byte(r[1], programmerType);
       SPI_CONTROLLER_Chip_Select_High(programmerType);
       usleep(1);

       SPI_CONTROLLER_Chip_Select_Low(programmerType);
       SPI_CONTROLLER_Write_One_Byte(0x04, programmerType);
       SPI_CONTROLLER_Chip_Select_High(programmerType);
       usleep(1);

       //Close the CH341a device
       ProgDeviceClose( programmerType );

       //Writing status registers 0,1 for not Winbond
       if (numOfRegisters > 0)
       {
           stCH341 = ProgDeviceInit( programmerType, 0, 1 );

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x06, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x50, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x01, programmerType);
           SPI_CONTROLLER_Write_One_Byte(r[0], programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x04, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           //Close the CH341a device
           ProgDeviceClose( programmerType );
           usleep(1);

           stCH341 = ProgDeviceInit( programmerType, 0, 1 );
           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x06, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x50, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x31, programmerType);
           SPI_CONTROLLER_Write_One_Byte(r[1], programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x04, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           //Close the CH341a device
           ProgDeviceClose( programmerType );
       }

       //Writing status register 2
       if (numOfRegisters > 1)
       {
           stCH341 = ProgDeviceInit( programmerType, 0, 1 );

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x06, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x50, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x11, programmerType);
           SPI_CONTROLLER_Write_One_Byte(r[2], programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           SPI_CONTROLLER_Chip_Select_Low(programmerType);
           SPI_CONTROLLER_Write_One_Byte(0x04, programmerType);
           SPI_CONTROLLER_Chip_Select_High(programmerType);
           usleep(1);

           //Close the CH341a device
           ProgDeviceClose( programmerType );
       }
   }
   else QMessageBox::about(this, tr("Error"), tr("Programmer ") + programmerName + tr(" is not connected!"));


  }
  else QMessageBox::about(this, tr("Error"), tr("Before writing the registers, please press the `Read` button!"));
}

void DialogSFDP::setLineEditFilter()
{
    QRegExp reHex( "[0-1]{1}" );
    QRegExpValidator *validator = new QRegExpValidator(reHex, this);
    //searching all lineEdit_srXX, XX - numbers
    QString searchText = "lineEdit_sr\\d+";
        QRegularExpression regex(searchText);

            for (QLineEdit* edit : findChildren<QLineEdit*>())
            {
                if (regex.match(edit->objectName()).hasMatch())
                {
                    edit->setValidator(validator);
                }
            }
}

void DialogSFDP:: setRegStatus(uint8_t regNumber, bool state)
{
    QString searchText = "lineEdit_sr" + QString::number(regNumber) + "\\d+";
    QRegularExpression regex(searchText);

        for (QLineEdit* edit : findChildren<QLineEdit*>())
        {
            if (regex.match(edit->objectName()).hasMatch())
            {
                if (state == false)
                {
                    edit->setDisabled(true);
                    edit->setText("");
                }
                else
                {
                    edit->setDisabled(false);
                }
            }
        }
        if (state == true)
        {
            if (regNumber == 1) ui->label_11->setDisabled(false);
            if (regNumber == 2) ui->label_34->setDisabled(false);
        }
        else
        {
            if (regNumber == 1) ui->label_11->setDisabled(true);
            if (regNumber == 2) ui->label_34->setDisabled(true);
        }
}

void DialogSFDP::closeEvent(QCloseEvent* event)
{
    emit closeRequestHasArrived();
    QWidget::closeEvent(event);
}

void DialogSFDP::on_pushButton_unlock_wps_clicked()
{
    // UNLOCK WPS: Clear Bit 2 (WPS) in SR3 using 0x11 command
    // Reference: W25Q128JV Datasheet, Write Status Register-3 (11h)
    
    if (numOfRegisters < 2) {
        QMessageBox::warning(this, tr("Warning"), 
            tr("SR3 (Status Register 3) is not available on this chip!"));
        return;
    }
    
    int stCH341 = ProgDeviceInit(programmerType, 0, 1);
    if (stCH341 != 0) {
        QMessageBox::about(this, tr("Error"), 
            tr("Programmer ") + programmerName + tr(" is not connected!"));
        return;
    }
    
    // Step 1: Read current SR3 value
    uint8_t current_sr3 = 0;
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x15, programmerType);  // Read SR3
    int retval = SPI_CONTROLLER_Read_NByte(&current_sr3, 1, SPI_CONTROLLER_SPEED_SINGLE, programmerType);
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    usleep(1);
    
    if (retval) {
        QMessageBox::about(this, tr("Error"), tr("Error reading SR3!"));
        ProgDeviceClose(programmerType);
        return;
    }
    
    // Step 2: Calculate new SR3 value with WPS bit (Bit 2) cleared
    // SR3 structure: [7:6]Reserved [5]HOLD/RST [4]SUS [3]CMP [2]WPS [1:0]DRV
    uint8_t new_sr3 = current_sr3 & ~(1 << 2);  // Clear Bit 2 (WPS)
    new_sr3 &= 0xFB;  // Force clear Bit 2, equivalent to &(~0x04)
    
    qDebug() << "Unlocking WPS: Current SR3 =" << Qt::hex << (int)current_sr3 
             << "-> New SR3 =" << (int)new_sr3;
    
    // Step 3: Send Write Enable command
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x06, programmerType);  // WREN
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    usleep(1);
    
    // Step 4: Send special write enable for status register
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x50, programmerType);  // Special WREN for SR
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    usleep(1);
    
    // Step 5: Write SR3 using 0x11 command (Write Status Register-3)
    // Command format: CS Low → 11h → SR3_value → CS High
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x11, programmerType);  // 0x11 - Write Status Register-3
    SPI_CONTROLLER_Write_One_Byte(new_sr3, programmerType);  // Write SR3 with WPS cleared
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    usleep(1);
    
    // Step 6: Send Write Disable command
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x04, programmerType);  // WRDI
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    usleep(1);
    
    // Step 7: Wait for write to complete
    usleep(10000);  // Wait 10ms
    
    // Step 8: Verify the write by reading SR3 again
    uint8_t verify_sr3 = 0;
    SPI_CONTROLLER_Chip_Select_Low(programmerType);
    SPI_CONTROLLER_Write_One_Byte(0x15, programmerType);  // Read SR3
    retval = SPI_CONTROLLER_Read_NByte(&verify_sr3, 1, SPI_CONTROLLER_SPEED_SINGLE, programmerType);
    SPI_CONTROLLER_Chip_Select_High(programmerType);
    
    ProgDeviceClose(programmerType);
    
    // Step 9: Check result and update UI
    if (!retval && verify_sr3 == new_sr3) {
        qDebug() << "WPS unlock successful! SR3 =" << Qt::hex << (int)verify_sr3;
        
        // Update UI to reflect the change
        printRegData(2, verify_sr3);  // Update SR3 display
        
        QMessageBox::information(this, tr("Success"), 
            tr("WPS bit has been cleared successfully!\n"
               "SR3 changed from 0x%1 to 0x%2")
                .arg(current_sr3, 0, 16)
                .arg(verify_sr3, 0, 16));
    } else {
        qDebug() << "WPS unlock failed! Expected:" << Qt::hex << (int)new_sr3 
                 << "Got:" << (int)verify_sr3;
        
        // Still update UI with current value
        if (!retval) {
            printRegData(2, verify_sr3);
        }
        
        QMessageBox::warning(this, tr("Warning"), 
            tr("Failed to clear WPS bit!\n"
               "Expected: 0x%1, Actual: 0x%2\n"
               "The chip may not support SR3 write operation.")
                .arg(new_sr3, 0, 16)
                .arg(verify_sr3, 0, 16));
    }
}

void DialogSFDP::printRegData(uint8_t regNumber, uint8_t regData)
{
    uint8_t i;
    int j;
    i = 128;

    for (j = 7; j >= 0; j--)
    {
        QString currRegName = "lineEdit_sr" + QString::number(regNumber) + QString::number(j);
        for (QLineEdit* edit : findChildren<QLineEdit*>())
        {
            if (edit->objectName() == currRegName)
            {
                edit->setText(QString::number(((regData & i) >> j)));
            }
        }
        i = i / 2;
    }
}

void DialogSFDP::setDeviceType(const uint8_t pType)
{
    programmerType = pType;
    if (programmerType < 2)  programmerName ="CH341A";
    if (programmerType == 2) programmerName ="CH347T";
}
