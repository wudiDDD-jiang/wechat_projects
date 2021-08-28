
QT       += core gui

INCLUDEPATH+=E:\OpenCV-QQT\include\opencv\
                    E:\OpenCV-QQT\include\opencv2\
                    E:\OpenCV-QQT\include

LIBS+=E:\OpenCV-QQT\lib\libopencv_calib3d2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_contrib2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_core2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_features2d2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_flann2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_gpu2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_highgui2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_imgproc2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_legacy2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_ml2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_objdetect2410.dll.a\
      E:\OpenCV-QQT\lib\libopencv_video2410.dll.a

HEADERS += \
    $$PWD/screen_read.h \
    $$PWD/video_read.h \
    $$PWD/common.h \
    $$PWD/myfacedetect.h

SOURCES += \
    $$PWD/screen_read.cpp \
    $$PWD/video_read.cpp \
    $$PWD/myfacedetect.cpp



