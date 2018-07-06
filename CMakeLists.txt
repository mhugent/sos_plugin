########################################################
# Files
find_package( Qt4 REQUIRED )

SET ( QGIS_DIR "C:/Program Files/QGIS 2 18" CACHE PATH "Path to QGIS installation")
SET ( EXPAT_INCLUDE_DIR "" CACHE PATH "Path to Expat include files" )
SET ( EXPAT_LIBRARY "" CACHE FILEPATH "Path to libexpat")
SET ( QWT_INCLUDE_DIR "" CACHE PATH "Path to QWT include files" )
SET ( QWT_LIBRARY "" CACHE FILEPATH "Path to QWT lib")
SET ( QT_INCLUDE_DIR "" CACHE PATH "Path to Qt include directory" )
SET ( QT_LIB_DIR "" CACHE PATH "Path to the directory containing the Qt libraries")

SET (CMAKE_CXX_STANDARD 11)


SET (SOS_SRCS
     qgssosplugin.cpp
     qgssossourceselect.cpp
     qgssoscapabilities.cpp
     qgsmaptoolsensorinfo.cpp
     qgssensorinfodialog.cpp
     qgswatermldata.cpp
     qgsxmldata.cpp
)

SET (SOS_UIS
    qgssossourceselectbase.ui
    qgssensorinfodialogbase.ui
    qgsnewhttpconnectionbase.ui
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

QT4_WRAP_UI (SOS_UIS_H  ${SOS_UIS})

QT4_WRAP_CPP( SOS_MOC_SRCS ${SOS_MOC_HDRS})

QT4_ADD_RESOURCES(SOS_RCC_SRCS ${SOS_RCCS})

ADD_LIBRARY (sosplugin MODULE
  ${SOS_SRCS}
  ${SOS_MOC_SRCS}
  ${SOS_UIS_H}
  ${SOS_RCC_SRCS}
 )

INCLUDE_DIRECTORIES(
     ${CMAKE_CURRENT_BINARY_DIR}
     .
     ${QGIS_DIR}/include
     ${QWT_INCLUDE_DIR}
     ${EXPAT_INCLUDE_DIR}
     ${QT_INCLUDE_DIR}/QtCore
     ${QT_INCLUDE_DIR}/QtGui
     ${QT_INCLUDE_DIR}/QtNetwork
     ${QT_INCLUDE_DIR}/QtXml
     ${QT_INCLUDE_DIR}/Qt
     ${QT_INCLUDE_DIR}
     ..
     .
)

IF (WIN32)
	ADD_DEFINITIONS( -DCORE_EXPORT=__declspec\(dllimport\))
	ADD_DEFINITIONS( -DGUI_EXPORT=__declspec\(dllimport\))
	ADD_DEFINITIONS( -Dnoexcept= )
ENDIF (WIN32)

TARGET_LINK_LIBRARIES(sosplugin
  ${QGIS_DIR}/lib/qgis_core.lib
  ${QGIS_DIR}/lib/qgis_gui.lib
  ${EXPAT_LIBRARY}
  ${QWT_LIBRARY}
  ${QT_LIB_DIR}/QtCore4.lib
  ${QT_LIB_DIR}/QtGui4.lib
  ${QT_LIB_DIR}/QtNetwork4.lib
  ${QT_LIB_DIR}/QtXml4.lib
)

INSTALL( TARGETS sosplugin DESTINATION . )
