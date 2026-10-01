import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
 id: root
 visible: false
 width: 520; height: 560
 flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
 title: "XLaunch"
 color: "#513b27"
 property string notice: ""
 property bool systemTheme: false
 property bool appLaunchMode: false
 readonly property color foreground: systemTheme ? palette.windowText : "#fff9eb"
 readonly property color secondary: systemTheme ? palette.placeholderText : "#dfd0ba"
 property real uiScale: Math.max(0.75, Math.min(width / 1448, height / 1086))
 Image { anchors.fill: parent; visible:!root.systemTheme && root.appLaunchMode;source: "../assets/amber-wallpaper.png"; fillMode: Image.PreserveAspectCrop }
 Rectangle { anchors.fill: parent; color: root.systemTheme?root.palette.window:(root.appLaunchMode?"#18000000":"#513b27") }

 signal layoutRequested()
 onAppLaunchModeChanged: layoutRequested()
 function focusMenu() { menuSearch.forceActiveFocus() }
 readonly property color menuBackground: systemTheme ? palette.window : "#513b27"
 readonly property color menuBase: systemTheme ? palette.base : "#3f2f21"
 readonly property color menuBorder: systemTheme ? palette.mid : "#756047"
 readonly property color menuHover: systemTheme ? palette.alternateBase : "#604b35"
 readonly property color menuSelection: systemTheme ? palette.highlight : "#796044"
 readonly property color menuFocus: systemTheme ? palette.highlight : "#dfbf8d"
 readonly property var categories: ["全部","网络","多媒体","游戏","图形","办公","开发","系统","工具","AI"]
 component AmberMenu: Menu {
  width:220
  palette.window:root.menuBackground;palette.text:root.foreground;palette.buttonText:root.foreground
  palette.highlight:root.menuSelection;palette.highlightedText:root.foreground
  background:Rectangle {implicitWidth:220;implicitHeight:40;color:root.menuBackground;border.color:root.menuBorder;radius:4}
 }
 component MenuAction: Button {
  id: control
  contentItem: Label { text: control.text; color: root.systemTheme && control.highlighted ? root.palette.highlightedText : root.foreground; verticalAlignment: Text.AlignVCenter; horizontalAlignment:Text.AlignHCenter; font.pixelSize:14 }
  background: Rectangle { radius:5; color:control.down?root.menuSelection:control.hovered||control.activeFocus?root.menuHover:root.menuBackground; border.color:control.activeFocus?root.menuFocus:root.menuBorder }
 }
 Rectangle {
  id: menuSurface; visible: !root.appLaunchMode; anchors.fill: parent
  color:root.menuBackground
  border.color:root.menuBorder
  ColumnLayout {
   anchors.fill:parent; anchors.margins:18; spacing:12
   RowLayout {
    Label { text:"XLaunch"; color:root.foreground; font.pixelSize:22; font.bold:true; Layout.fillWidth:true }
    MenuAction { text:"全部应用 ↗"; onClicked:{catalog.category="全部";catalog.query="";root.appLaunchMode=true;search.forceActiveFocus()} }
   }
   TextField {
    id:menuSearch; Layout.fillWidth:true; Layout.preferredHeight:40
    placeholderText:"搜索应用…"; placeholderTextColor:root.secondary; color:root.foreground; selectByMouse:true
    background:Rectangle {radius:5;color:root.menuBase;border.color:menuSearch.activeFocus?root.menuFocus:root.menuBorder}
    onTextChanged: if(!root.appLaunchMode){catalog.query=text;menuList.currentIndex=0}
    onAccepted:catalog.launch(menuList.currentIndex)
    Keys.onDownPressed:{menuList.forceActiveFocus();menuList.currentIndex=0}
   }
   Rectangle {Layout.fillWidth:true;Layout.preferredHeight:1;color:root.menuBorder}
   RowLayout {
    Layout.fillHeight:true; Layout.fillWidth:true; spacing:12
    ListView {
     Layout.preferredWidth:84;Layout.fillHeight:true;model:root.categories;spacing:2;clip:true
     delegate:MenuAction {
      required property string modelData
      width:84;height:34;text:modelData;highlighted:catalog.category===modelData
      background:Rectangle {radius:4;color:catalog.category===modelData?root.menuSelection:parent.hovered?root.menuHover:"transparent";border.color:parent.activeFocus?root.menuFocus:"transparent"}
      onClicked:{catalog.category=modelData;menuList.currentIndex=0}
     }
    }
    Rectangle {Layout.fillHeight:true;Layout.preferredWidth:1;color:root.menuBorder}
    Item {
     Layout.fillWidth:true;Layout.fillHeight:true
     ListView {
      id:menuList;anchors.fill:parent;model:root.appLaunchMode ? null : catalog;clip:true;spacing:2;reuseItems:true
      highlightMoveDuration:0;keyNavigationWraps:true
      ScrollBar.vertical:ScrollBar{}
      delegate:ItemDelegate {
       id:appRow
       required property int index;required property string appName;required property string appIcon
       width:menuList.width;height:42;enabled:!catalog.launching
       Accessible.name:appName
       contentItem:RowLayout {
        spacing:10
        Image {source:appRow.appIcon;sourceSize:Qt.size(32,32);Layout.preferredWidth:28;Layout.preferredHeight:28;fillMode:Image.PreserveAspectFit}
        Label {text:appRow.appName;color:root.foreground;elide:Text.ElideRight;Layout.fillWidth:true;font.pixelSize:14}
       }
       background:Rectangle {radius:4;color:appRow.hovered||menuList.currentIndex===appRow.index&&menuList.activeFocus?root.menuHover:"transparent";border.color:appRow.activeFocus?root.menuFocus:"transparent"}
       onClicked:{menuList.currentIndex=index;catalog.launch(index)}
       ToolTip.visible:hovered;ToolTip.delay:650;ToolTip.text:appName
       MouseArea {anchors.fill:parent;acceptedButtons:Qt.RightButton;onClicked:pinMenu.popup()}
       AmberMenu {id:pinMenu;popupType:Popup.Window;MenuItem {text:"在 Dock 中驻留";onTriggered:catalog.pin(appRow.index)}}
      }
      Keys.onReturnPressed:catalog.launch(currentIndex)
      Keys.onEnterPressed:catalog.launch(currentIndex)
     }
     Label {anchors.centerIn:parent;visible:catalog.count===0;text:"没有匹配的应用";color:root.secondary}
    }
   }
   Rectangle {Layout.fillWidth:true;Layout.preferredHeight:1;color:root.menuBorder}
   RowLayout {
    Label {text:catalog.launching?"正在打开…":root.notice.length?root.notice:catalog.count+" 个应用";color:root.secondary;Layout.fillWidth:true;elide:Text.ElideRight;font.pixelSize:12}
    MenuAction {text:"关闭";onClicked:root.hide()}
   }
  }
 }

 Label { visible: root.appLaunchMode; x: 26; y: 24; text: "XLaunch"; color: root.foreground; font.pixelSize: 19 }
 Button {
  id: menuButton
  visible: root.appLaunchMode
  anchors { top: parent.top; right: parent.right; topMargin: 18; rightMargin: 26 }
  text: "Menu"
  onClicked: { root.appLaunchMode = false; search.clear(); menuSearch.clear(); catalog.category = "全部";menuSearch.forceActiveFocus(); }
 }
 TextField {
  id: search
  visible: root.appLaunchMode
  anchors.top: parent.top; anchors.topMargin: 54 * root.uiScale
  anchors.horizontalCenter: parent.horizontalCenter
  width: Math.min(parent.width * 0.56, 780); height: 58 * root.uiScale
  color: root.foreground; font.pixelSize: 21 * root.uiScale
  placeholderText: "搜索应用…"; placeholderTextColor: root.secondary
  leftPadding: 22; rightPadding: 18; selectByMouse: true; focus: true
  background: Rectangle { radius: 8; color: root.systemTheme?root.palette.base:"#28ffffff"; border.color: root.systemTheme?(search.activeFocus?root.palette.highlight:root.palette.mid):(search.activeFocus ? "#b6dfc19a" : "#70ffffff"); border.width: 1 }
  onTextChanged: {catalog.query=text; grid.currentIndex=0; pages.currentIndex=0; root.notice=""}
  Keys.onDownPressed: {grid.forceActiveFocus();grid.currentIndex=0}
  onAccepted: catalog.launch(grid.currentIndex)
 }
 Label {
  visible: root.appLaunchMode
  anchors.top: search.bottom; anchors.topMargin: 16; anchors.horizontalCenter: search.horizontalCenter
  text: search.text.length ? catalog.count + " 个应用" : "搜索本机应用 · 右键驻留到 Dock"
  color: root.secondary;font.pixelSize: 14
 }
 RowLayout {
  visible: root.appLaunchMode
  anchors {left: parent.left;right:parent.right;top:search.bottom;bottom:footer.top;topMargin:54*root.uiScale;bottomMargin:20;leftMargin:26;rightMargin:30}
  spacing: 26
  ColumnLayout {
   Layout.preferredWidth: 184 * root.uiScale;Layout.maximumWidth:184 * root.uiScale;Layout.minimumWidth:184 * root.uiScale;Layout.fillHeight:true;spacing:8
   Repeater {
    model: ["全部","网络","多媒体","游戏","图形","办公","开发","系统","工具","AI"]
    delegate: Button {
     required property string modelData
     Layout.fillWidth:true;Layout.preferredHeight:58*root.uiScale
     text:modelData
     contentItem: Label {text:parent.text;color: root.systemTheme&&catalog.category===modelData?root.palette.highlightedText:root.foreground;font.pixelSize:21*root.uiScale;verticalAlignment:Text.AlignVCenter;leftPadding:22}
     background: Rectangle {radius:6;color:catalog.category===modelData?(root.systemTheme?root.palette.highlight:"#32ffffff"):parent.hovered?(root.systemTheme?root.palette.alternateBase:"#15ffffff"):"transparent";border.color:catalog.category===modelData?(root.systemTheme?root.palette.highlight:"#50fff2d6"):"transparent"}
     onClicked:{catalog.category=modelData;grid.currentIndex=0;pages.currentIndex=0}
    }
   }
   Item {Layout.fillHeight:true}
  }
  Rectangle {Layout.fillHeight:true;Layout.preferredWidth:1;color:root.systemTheme?root.palette.mid:"#45fce2b1"}
  Item {
   Layout.fillWidth:true;Layout.fillHeight:true
   Item {
    id:grid;anchors.fill:parent
    property int currentIndex:0
    readonly property int columns:Math.max(3,Math.min(9,Math.floor(width/140)))
    readonly property int pageSize:columns*5
    function select(delta) {
     currentIndex=Math.max(0,Math.min(catalog.count-1,currentIndex+delta))
     pages.currentIndex=Math.floor(currentIndex/pageSize)
    }
    ListView {
     id:pages;anchors {fill:parent;bottomMargin:30} clip:true
     orientation:ListView.Horizontal;snapMode:ListView.SnapOneItem
     boundsBehavior:Flickable.StopAtBounds;highlightRangeMode:ListView.StrictlyEnforceRange
     highlightMoveDuration:0;cacheBuffer:0;reuseItems:true;model:root.appLaunchMode ? Math.ceil(catalog.count/grid.pageSize) : 0
     onMovementEnded:grid.currentIndex=currentIndex*grid.pageSize
     delegate:Item {
      id:pageItem;required property int index;width:pages.width;height:pages.height
      Grid {
       anchors.fill:parent;columns:grid.columns
       Repeater {
        model:{let revision=catalog.revision;return catalog.page(pageItem.index*grid.pageSize,grid.pageSize)}
        delegate:Item {
         required property var modelData
         width:pages.width/grid.columns;height:pages.height/5
         Accessible.role:Accessible.Button;Accessible.name:modelData.name
         Accessible.onPressAction:catalog.launch(modelData.row)
         Rectangle {anchors.fill:parent;anchors.margins:6;radius:5;color:(grid.currentIndex===modelData.row&&grid.activeFocus)||mouse.containsMouse?(root.systemTheme?root.palette.alternateBase:"#20ffffff"):"transparent";border.color:grid.currentIndex===modelData.row&&grid.activeFocus?(root.systemTheme?root.palette.highlight:"#80ffffff"):"transparent"}
         Image {anchors.horizontalCenter:parent.horizontalCenter;y:8*root.uiScale;width:76*root.uiScale;height:76*root.uiScale;source:modelData.icon;sourceSize:Qt.size(96,96);fillMode:Image.PreserveAspectFit}
         Label {anchors.horizontalCenter:parent.horizontalCenter;y:94*root.uiScale;width:parent.width-10;text:modelData.name;color:root.foreground;font.pixelSize:17*root.uiScale;horizontalAlignment:Text.AlignHCenter;elide:Text.ElideRight;maximumLineCount:2;wrapMode:Text.Wrap}
         AmberMenu {id:gridPinMenu;popupType:Popup.Window;MenuItem{text:"在 Dock 中驻留";onTriggered:catalog.pin(modelData.row)}}
         MouseArea {id:mouse;anchors.fill:parent;hoverEnabled:true;acceptedButtons:Qt.LeftButton|Qt.RightButton;onClicked:function(event){grid.currentIndex=modelData.row;if(event.button===Qt.RightButton)gridPinMenu.popup();else catalog.launch(modelData.row)}}
        }
       }
      }
     }
    }
    PageIndicator {id:pageDots;anchors.bottom:parent.bottom;anchors.horizontalCenter:parent.horizontalCenter;count:pages.count;currentIndex:pages.currentIndex;interactive:true;onCurrentIndexChanged:{pages.currentIndex=currentIndex;if(Math.floor(grid.currentIndex/grid.pageSize)!==currentIndex)grid.currentIndex=currentIndex*grid.pageSize}
     delegate:Rectangle {required property int index;implicitWidth:8;implicitHeight:8;radius:4;color:root.foreground;opacity:index===pageDots.currentIndex?0.95:0.35}}
    Keys.onLeftPressed:select(-1)
    Keys.onRightPressed:select(1)
    Keys.onUpPressed:select(-columns)
    Keys.onDownPressed:select(columns)
    Keys.onReturnPressed:catalog.launch(currentIndex)
    Keys.onEnterPressed:catalog.launch(currentIndex)
    Keys.onPressed:function(event){if(event.text.length&&!(event.modifiers&(Qt.ControlModifier|Qt.MetaModifier))){search.forceActiveFocus();search.text+=event.text;event.accepted=true}}
   }
   Label {anchors.centerIn:parent;visible:catalog.count===0;text:catalog.category==="AI"?"Agent 尚未连接\n请切换到“全部”浏览本机应用": "没有匹配的应用\n尝试其他名称或分类";color:root.foreground;font.pixelSize:20;horizontalAlignment:Text.AlignHCenter}
  }
 }
 Label {id:footer;visible:root.appLaunchMode;anchors.bottom:parent.bottom;anchors.bottomMargin:28;anchors.horizontalCenter:parent.horizontalCenter;text:root.notice.length?root.notice:"左右拖动翻页     方向键 选择     Enter 打开     Esc 返回 Menu";color:root.secondary;font.pixelSize:16}
 Shortcut {sequence:"Escape";onActivated:{if(catalog.query.length){search.clear();menuSearch.clear()}else if(root.appLaunchMode){root.appLaunchMode=false}else root.hide()}}

 Shortcut {sequence:"F11";onActivated:{root.appLaunchMode=!root.appLaunchMode;if(root.appLaunchMode)search.forceActiveFocus()}}
 Shortcut {sequence:"Ctrl+F";onActivated:root.appLaunchMode?search.forceActiveFocus():menuSearch.forceActiveFocus()}
 Shortcut {sequence:"Meta+F";onActivated:root.appLaunchMode?search.forceActiveFocus():menuSearch.forceActiveFocus()}
 Shortcut {sequence:"Ctrl+R";onActivated:catalog.refresh()}
 onVisibleChanged:if(visible){search.clear();menuSearch.clear();catalog.category="全部";grid.currentIndex=0;pages.currentIndex=0}
 Shortcut {sequence:"Ctrl+Right";onActivated:{pages.currentIndex=Math.min(pages.count-1,pages.currentIndex+1);grid.currentIndex=pages.currentIndex*grid.pageSize}}
 Shortcut {sequence:"Ctrl+Left";onActivated:{pages.currentIndex=Math.max(0,pages.currentIndex-1);grid.currentIndex=pages.currentIndex*grid.pageSize}}
 Connections {target:catalog;function onFailure(message){root.notice=message}}
}
