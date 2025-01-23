import QtQuick 2.15

Item {
    id: root
    
    property var someObject: null
    
    function processData() {
        if (someObject == null) {  // Should trigger warning
            console.log("Object is null");
        }
        
        // Complex JS code
        let array = [1, 2, 3];
        array.forEach(item => {
            if (item != 2) {  // Should trigger warning
                console.log(item);
            }
        });
    }
    
    Component.onCompleted: {
        let x = 10;
        if (x == 10) {  // Should trigger warning
            processData();
        }
    }
}