includes the project code at various gateways, the node.js project directory, the functions used to createData, code to analyze the latency of functions, and the exported database JSON as of 08/12/23.

CODE LOGIC:

Card Detection: It checks if a new card is present (isNewCardPresent) and if the card is successfully read (isCardRead). If both conditions are met, it resets the absent counter and handles card addition. If the card was not present before, it prints card information and sets the LED color. If no new card is detected or there's a read failure, it increments the absent counter and handles card removal.

Object Detection: This section checks for object presence based on a time interval. It calculates objectDistance and checks if an object is present (isObjThere). If an object is detected (distance <= 60), it handles object addition, prints a message, and sets the LED color.

Card Presence State Change: It checks if the card presence state has changed compared to the previous iteration and pushes a message to the database accordingly.

Timer Expiration Logic: If there's no card present but an object is detected, it checks if the timer has expired. If the timer has expired, it sets the LED color and may send an email. Otherwise, it resets the LED color and timer state.
LED Color Control: This section sets the LED color based on various conditions, including card and object presence. Light State Change: It checks if the LED color state has changed and updates the state in the database.
