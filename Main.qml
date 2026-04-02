import QtQuick
import QtQuick.Controls
import QtQuick3D
import QtQuick3D.Helpers
import SatelliteTracker

ApplicationWindow {
    visible: true
    width: 800
    height: 600
    title: "ISS Satellite Tracker 3D"

    SatelliteTracker {
        id: tracker
    }

    View3D {
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#0a0a1a"
            backgroundMode: SceneEnvironment.Color
        }

        OrbitCameraController {
            id: orbitController
            camera: camera
            origin: earthModel
            panEnabled: false
        }

        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 0, 700)
            clipNear: 1
            clipFar: 5000
        }

        DirectionalLight {
            eulerRotation: Qt.vector3d(
                Math.atan2(tracker.sunY, Math.sqrt(tracker.sunX * tracker.sunX + tracker.sunZ * tracker.sunZ)) * (180 / Math.PI),
                Math.atan2(tracker.sunX, tracker.sunZ) * (180 / Math.PI),
                0
            )
            brightness: 2.0
            color: "#fffde7"
        }

        DirectionalLight {
            eulerRotation.x: 10
            eulerRotation.y: 150
            brightness: 0.02
            color: "#223366"
        }

        // Sun sphere
        Model {
            source: "#Sphere"
            position: Qt.vector3d(tracker.sunX * 500, tracker.sunY * 500, tracker.sunZ * 500)
            scale: Qt.vector3d(0.8, 0.8, 0.8)
            materials: PrincipledMaterial {
                baseColor: "#fffde7"
                emissiveFactor: Qt.vector3d(1.0, 0.95, 0.6)
            }
        }

        // Sun glow
        Model {
            source: "#Sphere"
            position: Qt.vector3d(tracker.sunX * 500, tracker.sunY * 500, tracker.sunZ * 500)
            scale: Qt.vector3d(1.1, 1.1, 1.1)
            materials: PrincipledMaterial {
                baseColor: "#ffcc00"
                emissiveFactor: Qt.vector3d(0.8, 0.6, 0.0)
                opacity: 0.15
                alphaMode: PrincipledMaterial.Blend
            }
        }

        // Earth
        Model {
            id: earthModel
            source: "#Sphere"
            scale: Qt.vector3d(2, 2, 2)
            eulerRotation.y: tracker.earthRotationAngle
            materials: PrincipledMaterial {
                baseColorMap: Texture { source: "qrc:/qt/qml/SatelliteTracker/earth.jpg" }
                emissiveMap: Texture { source: "qrc:/qt/qml/SatelliteTracker/earth_night.jpg" }
                emissiveFactor: Qt.vector3d(0.8, 0.8, 0.8)
                roughness: 0.8
                metalness: 0.0
            }
        }

        // Past track — red fading dots
        Repeater3D {
            model: tracker.pastTrack
            delegate: Model {
                required property var modelData
                position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                source: "#Sphere"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    baseColor: "#ff3333"
                    emissiveFactor: Qt.vector3d(modelData.opacity, 0.1, 0.1)
                    opacity: modelData.opacity
                    alphaMode: PrincipledMaterial.Blend
                }
            }
        }

        // Future track — blue fading dots
        Repeater3D {
            model: tracker.futureTrack
            delegate: Model {
                required property var modelData
                position: Qt.vector3d(modelData.x, modelData.y, modelData.z)
                source: "#Sphere"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    baseColor: "#3388ff"
                    emissiveFactor: Qt.vector3d(0.1, 0.3, modelData.opacity)
                    opacity: modelData.opacity
                    alphaMode: PrincipledMaterial.Blend
                }
            }
        }

        // Satellite
        Node {
            id: satellite
            x: tracker.x
            y: tracker.y
            z: tracker.z

            Behavior on x { SmoothedAnimation { velocity: 50 } }
            Behavior on y { SmoothedAnimation { velocity: 50 } }
            Behavior on z { SmoothedAnimation { velocity: 50 } }

            // Glowing core dot
            Model {
                source: "#Sphere"
                scale: Qt.vector3d(0.1, 0.1, 0.1)
                materials: PrincipledMaterial {
                    baseColor: "#ffffff"
                    emissiveFactor: Qt.vector3d(1.0, 0.6, 0.0)
                }
            }

            // Billboard satellite image
            Model {
                source: "#Rectangle"
                scale: Qt.vector3d(0.3, 0.3, 0.3)
                eulerRotation: Qt.vector3d(
                    -Math.atan2(
                        camera.position.y - satellite.y,
                        Math.sqrt(Math.pow(camera.position.x - satellite.x, 2) +
                                  Math.pow(camera.position.z - satellite.z, 2))
                    ) * (180 / Math.PI),
                    Math.atan2(
                        camera.position.x - satellite.x,
                        camera.position.z - satellite.z
                    ) * (180 / Math.PI),
                    0
                )
                materials: PrincipledMaterial {
                    baseColorMap: Texture { source: "qrc:/qt/qml/SatelliteTracker/satellite.png" }
                    alphaMode: PrincipledMaterial.Blend
                    roughness: 1.0
                    metalness: 0.0
                    emissiveFactor: Qt.vector3d(0.5, 0.5, 0.5)
                    emissiveMap: Texture { source: "qrc:/qt/qml/SatelliteTracker/satellite.png" }
                }
            }
        }
    }

    // Telemetry panel
    Rectangle {
        anchors { left: parent.left; bottom: parent.bottom; margins: 12 }
        width: 260; height: 168
        color: "#88000000"
        radius: 6

        Column {
            anchors { fill: parent; margins: 10 }
            spacing: 5

            Label { text: "🛰  ISS Live Tracker"; color: "white"; font.bold: true; font.pixelSize: 13 }
            Label { text: tracker.statusText; color: "#88ccff"; font.pixelSize: 11 }
            Label { text: "Alt: " + tracker.altitude.toFixed(0) + " km  |  Vel: " + tracker.velocity.toFixed(2) + " km/s"; color: "#88ccff"; font.pixelSize: 11 }

            Label {
                id: utcLabel
                color: "#aaffaa"
                font.pixelSize: 11
                function getUTC() {
                    var d = new Date()
                    var h = String(d.getUTCHours()).padStart(2, '0')
                    var m = String(d.getUTCMinutes()).padStart(2, '0')
                    var s = String(d.getUTCSeconds()).padStart(2, '0')
                    return "UTC: " + h + ":" + m + ":" + s
                }
                text: getUTC()
                Timer { interval: 1000; running: true; repeat: true; onTriggered: utcLabel.text = utcLabel.getUTC() }
            }

            Label { text: "Earth °: " + tracker.earthRotationAngle.toFixed(2) + "°"; color: "#aaffaa"; font.pixelSize: 11 }
            Label { text: "Rot. speed: 0.00417°/s  |  15°/hr"; color: "#aaffaa"; font.pixelSize: 11 }
            Label { text: "Drag to orbit • Scroll to zoom"; color: "#666"; font.pixelSize: 10 }
        }
    }

    // Live dot
    Rectangle {
        anchors { right: parent.right; top: parent.top; margins: 12 }
        width: 10; height: 10; radius: 5
        color: "#00ff88"
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { to: 0.2; duration: 800 }
            NumberAnimation { to: 1.0; duration: 800 }
        }
    }

    // Reset view
    Rectangle {
        anchors { right: parent.right; top: parent.top; topMargin: 36; rightMargin: 12 }
        width: 80; height: 28
        color: "#88000000"; radius: 5
        border.color: "#44ffffff"; border.width: 1
        Text { anchors.centerIn: parent; text: "Reset View"; color: "white"; font.pixelSize: 11 }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                camera.position = Qt.vector3d(0, 0, 700)
                camera.eulerRotation = Qt.vector3d(0, 0, 0)
            }
        }
    }
}