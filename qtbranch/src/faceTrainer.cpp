/* TRANSLATOR std::faceTrainer */

/*
    QT Face Trainer MAIN
    Copyright (C) 2010 Rohan Anil (rohan.anil@gmail.com) -BITS Pilani Goa Campus
    http://www.pam-face-authentication.org

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtGui>
#include <QApplication>

#include <iostream>
#include <list>
#include <string>
#include <cctype>
#include <dirent.h>
#include <unistd.h>

#include "faceTrainer.h"
#include "faceTrainerAdvSettings.h"
#include "aboutBox.h"
#include "webcamImagePaint.h"
#include "utils.h"
#include "cv.h"
#include "highgui.h"
#include "qtUtils.h"
#include "pam_face_defines.h"

//------------------------------------------------------------------------------
faceTrainer::faceTrainer(QWidget* parent) : QMainWindow(parent)
{
    ui.setupUi(this);
    QDesktopWidget* desktop = QApplication::desktop();

    int screenWidth = desktop->width(); // get width of screen
    int screenHeight = desktop->height(); // get height of screen

    QSize windowSize = size(); // size of our application window
    int width = windowSize.width();
    int height = windowSize.height();

    // little computations
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    y -= 50;

    // move window to desired coordinates
    move(x, y);

    ui.stkWg->setCurrentIndex(0);
    connect(ui.pb_capture, SIGNAL(clicked()), this, SLOT(captureClick()));
    connect(ui.pb_next_t2, SIGNAL(clicked()), this, SLOT(showTab3()));
    connect(ui.pb_next_t1, SIGNAL(clicked()), this, SLOT(showTab2()));
    connect(ui.pb_back_t2, SIGNAL(clicked()), this, SLOT(showTab1()));
    connect(ui.pb_about, SIGNAL(clicked()), this, SLOT(about()));
    connect(ui.pb_ds, SIGNAL(clicked()), this, SLOT(removeSelected()));
    connect(ui.pb_adv, SIGNAL(clicked()), this, SLOT(showAdvDialog()));

    // connect(ui.button1, SIGNAL(clicked()), this, SLOT(verify()));
    // connect(ui.but, SIGNAL(clicked()), this, SLOT(butClick()));
}

//------------------------------------------------------------------------------
faceTrainer::~faceTrainer()
{
}

//------------------------------------------------------------------------------
void faceTrainer::showTab1()
{
    killTimer(timerId);
    webcam.stopCamera();

    ui.stkWg->setCurrentIndex(0);
}

//------------------------------------------------------------------------------
void faceTrainer::showTab2()
{
    if(webcam.startCamera() == true)
    {
        ui.stkWg->setCurrentIndex(1);
        populateQList();
        timerId = startTimer(20);
    }
    else
    {
        QMessageBox msgBox1;
        msgBox1.setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);"));
        msgBox1.setWindowTitle(tr("Face Trainer"));
        msgBox1.setText(tr("<strong>Error</strong>"));
        msgBox1.setInformativeText(tr(
                                       "Camera Not Found. <br /> "
                                       "Plugin Your Camera and Try Again."));
        msgBox1.setStandardButtons(QMessageBox::Ok);
        msgBox1.setIconPixmap(QPixmap(":/data/ui/images/cnc.png"));
        msgBox1.exec();
    }
}

//------------------------------------------------------------------------------
void faceTrainer::showTab3()
{
    ui.stkWg->setCurrentIndex(2);
}

//------------------------------------------------------------------------------
void faceTrainer::populateQList()
{
    ui.lv_thumbnails->clear();
    setFace* faceSetStruct = newVerifier.getFaceSet();

    for(int i = 0; i < faceSetStruct->count; i++)
    {
        char setName[100];
        sprintf(setName, "Set %d", i+1);
        // ui.lv_thumbnails->setIconSize(QSize(60, 60));
        
        QListWidgetItem *item = new QListWidgetItem(setName, ui.lv_thumbnails);
        item->setIcon(QIcon(faceSetStruct->setFilePathThumbnails[i]));
        QString qstring(faceSetStruct->setName[i]);
        // cout << faceSetStruct->setName[i] << endl;
        item->setData(Qt::UserRole, qstring);
    }
}

//------------------------------------------------------------------------------
void faceTrainer::setQImageWebcam(QImage* input)
{
    if(!input) return;

    static QGraphicsScene* scene = new QGraphicsScene(0, 0, IMAGE_WIDTH, IMAGE_HEIGHT);
    ui.webcamPreview->setScene(scene);
    ui.webcamPreview->setBackgroundBrush(*input);

    ui.webcamPreview->show();

}

//------------------------------------------------------------------------------
aboutBox::aboutBox(QWidget* parent) : QDialog(parent)
{
    ui.setupUi(this);
}

//------------------------------------------------------------------------------
void faceTrainer::about()
{
    aboutBox newAboutBox;
    newAboutBox.exec();
}

//------------------------------------------------------------------------------
void faceTrainer::showAdvDialog()
{
    faceTrainerAdvSettings* newDialog = new faceTrainerAdvSettings(
        this, newVerifier.configDirectory);
    newDialog->initConfig();
    newDialog->sT(&webcam, &newDetector, &newVerifier);
    newDialog->exec();
    delete newDialog;
}

//------------------------------------------------------------------------------
/// Currently unused!
void faceTrainer::verify()
{
    struct dirent* de = NULL;

    DIR* d = opendir("/home/notroot/train/");
    while(de = readdir(d))
    {
        if( !((strcmp(de->d_name, ".") == 0) 
           || (strcmp(de->d_name, "..") == 0)) )
        {
            char fullPath[300];
            sprintf(fullPath,"%s/%s","/home/notroot/train",de->d_name);
            IplImage* temp = cvLoadImage(fullPath, 1);
            printf("%s\n", fullPath);
            if(newVerifier.verifyFace(temp) == true) printf("%s\n", fullPath);
        }
    }

    //IplImage* queryImage = webcam.queryFrame();
    //newVerifier.verifyFace(queryImage);
}

/*
//------------------------------------------------------------------------------
void faceTrainer::butClick()
{
    IplImage* queryImage = webcam.queryFrame();
    newVerifier.verifyFace(newDetector.clipFace(queryImage));
}
*/

//------------------------------------------------------------------------------
void faceTrainer::removeSelected()
{
    QList<QListWidgetItem*> list = ui.lv_thumbnails->selectedItems();
    QList<QListWidgetItem*>::iterator i;

    for(i = list.begin(); i != list.end(); ++i)
    {
        QListWidgetItem* item = *i;
        QString dat = item->data(Qt::UserRole).toString();
        
        char *ptr = dat.toAscii().data();
        // printf("%s\n", ptr);

        newVerifier.removeFaceSet(ptr);
    }
    ui.lv_thumbnails->clear();

    populateQList();
}

//------------------------------------------------------------------------------
void faceTrainer::captureClick()
{
    static int latch = 0;
    
    if(latch == 0)
    {
        ui.pb_capture->setText(tr("Cancel"));
        latch = 1;
        newDetector.startClipFace(13);
    }
    else
    {
        ui.pb_capture->setText(tr("Capture"));
        latch = 0;
        newDetector.stopClipFace();
    }
}

//------------------------------------------------------------------------------
QString faceTrainer::getQString(int messageIndex)
{
    /* sprintf(messageCaptureMessage, 
        "Captured %d/%d faces.",
        totalFaceClipNum - clipFaceCounter + 1,
        totalFaceClipNum);*/

    if (messageIndex == -1) return QString("");
    else if (messageIndex == 0)
        return QString(tr("Please come closer to the camera."));
    else if (messageIndex == 1)
        return QString(tr("Please go little far from the camera."));
    else if (messageIndex == 2)
        return QString(tr("Unable to Detect Your Face."));
    else if (messageIndex == 3)
        return QString(tr("Tracker lost, trying to reinitialize."));
    else if (messageIndex == 4)
        return QString(tr("Tracking in progress."));
    else if (messageIndex == 5)
        return QString(tr("Captured %1 / %2 faces.").arg(14-newDetector.getClipFaceCounter()).arg(13));
    else if (messageIndex == 6)
        return QString(tr("Capturing Image Finished."));

    return 0;
}

//------------------------------------------------------------------------------
void faceTrainer::setIbarText(QString message)
{
    ui.lbl_ibar->setText(message);
}

//------------------------------------------------------------------------------
void faceTrainer::timerEvent(QTimerEvent*)
{
    static webcamImagePaint newWebcamImagePaint;
    
    IplImage* queryImage = webcam.queryFrame();
    newDetector.runDetector(queryImage);

    setIbarText(getQString(newDetector.queryMessage()));
    //if(newDetector.checkEyeDetected()==1)
    //newVerifier.verifyFace(newDetector.clipFace(queryImage));
    //this works captureClick();
    //double t = (double)cvGetTickCount();
    
    newWebcamImagePaint.paintCyclops(queryImage, 
        newDetector.eyesInformation.LE, newDetector.eyesInformation.RE);
    newWebcamImagePaint.paintEllipse(queryImage, 
        newDetector.eyesInformation.LE, newDetector.eyesInformation.RE);

    /* cvLine(queryImage, 
          newDetector.eyesInformation.LE, newDetector.eyesInformation.RE, 
          cvScalar(0,255,0), 4);*/
    // newVerifier.verifyFace(newDetector.clipFace(queryImage));
    QImage* qm = QImageIplImageCvt(queryImage);

    if(newDetector.finishedClipFace() == true)
    {
        setIbarText(tr("Processing Faces, Please Wait ..."));
        // cvWaitKey(1000);
        newVerifier.addFaceSet(newDetector.returnClipedFace(), 13);
        setIbarText(tr("Processing Completed."));

        captureClick();
        populateQList();
    }

    setQImageWebcam(qm);

    cvWaitKey(1);
    // sleep(1);

    delete qm;
    cvReleaseImage(&queryImage);
}

