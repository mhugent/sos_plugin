########################################################
# Files
CMAKE_MINIMUM_REQUIRED (VERSION 2.8.8)

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Xml REQUIRED)

SET ( QGIS_INCLUDE_DIR "C:/Program Files/QGIS 2 18/include" CACHE PATH "Path to QGIS include directory")
SET ( QGIS_CORE_LIB "C:/Program Files/QGIS 2 18/lib/qgis_core.lib" CACHE PATH "QGIS core library" )
SET ( QGIS_GUI_LIB "C:/Program Files/QGIS 2 18/lib/qgis_gui.lib" CACHE PATH "QGIS gui library" )
SET ( EXPAT_INCLUDE_DIR "" CACHE PATH "Path to Expat include files" )
SET ( EXPAT_LIBRARY "" CACHE FILEPATH "Path to libexpat")
SET ( QWT_INCLUDE_DIR "" CACHE PATH "Path to QWT include files" )
SET ( QWT_LIBRARY "" CACHE FILEPATH "Path to QWT lib")
SET ( QT_INCLUDE_DIR "" CACHE PATH "Path to Qt include directory" )
SET ( QT_LIB_DIR "" CACHE PATH "Path to the directory containing the Qt libraries")

SET (CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)

SET (SOS_SRCS
     qgssosplugin.cpp
     qgssosconnectiondialog.cpp
     qgssossourceselect.cpp
     qgssoscapabilities.cpp
     qgsmaptoolsensorinfo.cpp
     qgssensorinfodialog.cpp
     qgswatermldata.cpp
     qgsxmldata.cpp
)

SET (SOS_UIS
    qgssosconnectiondialogbase.ui
    qgssossourceselectbase.ui
    qgssensorinfodialogbase.ui
    )

SET (SOS_MOC_HDRS
qgssosplugin.h
qgssossourceselect.h
qgssoscapabilities.h
qgsmaptoolsensorinfo.h
qgssensorinfodialog.h
qgsxmldata.h
)

IF (WIN32)
	ADD_DEFINITIONS(-DQWT_DLL=1)
ENDIF (WIN32)

SET (SOS_RCCS  resources.qrc)
########################################################
# Build

QT5_WRAP_UI (SOS_UIS_H  ${SOS_UIS})

QT5_ADD_RESOURCES(SOS_RCC_SRCS ${SOS_RCCS})

ADD_LIBRARY (sosplugin MODULE
  ${SOS_SRCS}
  ${SOS_MOC_SRCS}
  ${SOS_UIS_H}
  ${SOS_RCC_SRCS}
 )

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     .
     ${QGIS_INCLUDE_DIR}
     ${QWT_INCLUDE_DIR}
     ${EXPAT_INCLUDE_DIR}
     ${QT_INCLUDE_DIR}/QtCore
     ${QT_INCLUDE_DIR}/QtGui
     ${QT_INCLUDE_DIR}/QtNetwork
     ${QT_INCLUDE_DIR}/QtWidgets
     ${QT_INCLUDE_DIR}/QtXml
     ${QT_INCLUDE_DIR}/QtSvg
     ${QT_INCLUDE_DIR}/Qt
     ${QT_INCLUDE_DIR}
     ..
     .
)

IF (WIN32)
	ADD_DEFINITIONS( -DCORE_EXPORT=__declspec\(dllimport\))
	ADD_DEFINITIONS( -DGUI_EXPORT=__declspec\(dllimport\))
	ADD_DEFINITIONS( -Dnoexcept= )
ELSE()
  ADD_DEFINITIONS( -DCORE_EXPORT= )
	ADD_DEFINITIONS( -DGUI_EXPORT= )
ENDIF (WIN32)

IF (WIN32)
TARGET_LINK_LIBRARIES(sosplugin
  ${QGIS_CORE_LIB}
  ${QGIS_GUI_LIB}
  ${EXPAT_LIBRARY}
  ${QWT_LIBRARY}
  ${QT_LIB_DIR}/QtCore5.lib
  ${QT_LIB_DIR}/QtGui5.lib
  ${QT_LIB_DIR}/QtNetwork5.lib
  ${QT_LIB_DIR}/QtWidgets5.lib
  ${QT_LIB_DIR}/QtXml5.lib
)
ELSE()
  TARGET_LINK_LIBRARIES(sosplugin
  ${QGIS_CORE_LIB}
  ${QGIS_GUI_LIB}
  ${EXPAT_LIBRARY}
  ${QWT_LIBRARY}
  ${QT_LIB_DIR}/libQt5Core.so
  ${QT_LIB_DIR}/libQt5Gui.so
  ${QT_LIB_DIR}/libQt5Network.so
  ${QT_LIB_DIR}/libQt5Widgets.so
  ${QT_LIB_DIR}/libQt5Xml.so
)
ENDIF (WIN32)

INSTALL( TARGETS sosplugin DESTINATION . )
