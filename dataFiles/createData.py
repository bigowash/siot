import random
import numpy as np
import datetime
import json

def create_inputs():
    # Constants
    available_cards = ["C4 8F 1F 2E", "33 76 EA FD", "2H 34 23 HF", "55 H4 IE 9I", "DS 3D 1N MM"]
    num_events = 10  # Adjust the number of events

    # Start time set to 7 AM
    start_time = datetime.datetime(2023, 1, 1, 7, 0).timestamp() * 1000  # in milliseconds

    # Generate inputs
    card_uids = []
    timer_expired = []
    minutes_card_placed = []
    minutes_in_between_cards = []
    object_present = False

    for i in range(num_events):
        # Randomly decide if an object is added or removed
        object_event = random.choice(['add', 'remove'])
        if object_event == 'add':
            object_present = True
        elif object_event == 'remove' and object_present:
            object_present = False

        card_uid = random.choice(available_cards)
        card_uids.append(card_uid)

        # Generate minutes_card_placed using a flattened normal distribution and convert to milliseconds
        duration = int(abs(np.random.normal(100, 60))) + random.random()
        minutes_card_placed.append(duration)

        # Start timer if object is present and card is removed
        if object_present and random.random() < 0.5:
            # Simulate card removed
            timer_expired.append(True)
            object_present = False  # Object gets removed when timer starts
        else:
            timer_expired.append(False)

        if i < num_events - 1:
            # Generate minutes_in_between_cards and convert to milliseconds
            minutes_in_between_cards.append(random.randint(0, 275) + random.random())

    return start_time, card_uids, timer_expired, minutes_card_placed, minutes_in_between_cards

# Example usage
start_time, card_uids, timer_expired, minutes_card_placed, minutes_in_between_cards = create_inputs()

# print("start_time = ", start_time)
# print("card_uids = ", card_uids)
# print("timer_expired = ", timer_expired)
# print("minutes_card_placed = ", minutes_card_placed)
# print("minutes_in_between_cards = ", minutes_in_between_cards)

def create_json_object(start_time, card_uids, timer_expired, minutes_card_placed, minutes_in_between_cards):
    # Conversion of minutes to milliseconds
    one_minute_ms = 60 * 1000

    # Initial timestamp
    current_timestamp = start_time

    # JSON object to store the events
    json_object = {}

    # Function to generate unique IDs
    def generate_id(index, subindex):
        return f"-NkVG{chr(ord('a') + index)}{chr(ord('a') + subindex)}"

    # Iterate through the cards and their respective times
    for i, (card_uid, is_timer_expired) in enumerate(zip(card_uids, timer_expired)):
        # Calculate timestamps for 'Object Added' and 'Card Placed'
        if random.choice([True, False]):
            object_added_timestamp = current_timestamp - random.randint(0, 2) * one_minute_ms
            card_placed_timestamp = current_timestamp
        else:
            card_placed_timestamp = current_timestamp
            object_added_timestamp = current_timestamp + random.randint(0, 2) * one_minute_ms

        json_object[generate_id(i, 0)] = {"event": "Object Added", "timestamp": object_added_timestamp}
        json_object[generate_id(i, 1)] = {"cardUID": card_uid.strip(), "event": "Card Placed", "timestamp": card_placed_timestamp}

        # Calculate timestamp for 'Card Removed'
        card_removed_timestamp = current_timestamp + minutes_card_placed[i] * one_minute_ms
        json_object[generate_id(i, 2)] = {"event": "Card Removed", "timestamp": card_removed_timestamp}

        # Handle timer expiration
        if is_timer_expired:
            timer_expired_timestamp = card_removed_timestamp + 15 * one_minute_ms
            object_removed_timestamp = timer_expired_timestamp + random.randint(5, 110) * one_minute_ms
            json_object[generate_id(i, 3)] = {"event": "Timer Expired", "timestamp": timer_expired_timestamp}
            json_object[generate_id(i, 4)] = {"event": "Object Removed", "timestamp": object_removed_timestamp}
            current_timestamp = object_removed_timestamp
        else:
            # Randomize the placement of 'Object Removed' event
            if random.choice([True, False]):
                object_removed_timestamp = card_removed_timestamp - random.randint(0, 2) * one_minute_ms
            else:
                object_removed_timestamp = card_removed_timestamp + random.randint(0, 2) * one_minute_ms

            json_object[generate_id(i, 3)] = {"event": "Object Removed", "timestamp": object_removed_timestamp}
            # Update the current timestamp for the next card (if any)
            if i < len(minutes_in_between_cards):
                current_timestamp = card_removed_timestamp + minutes_in_between_cards[i] * one_minute_ms

    return json_object


# Create the JSON object
json_object = create_json_object(start_time, card_uids, timer_expired, minutes_card_placed, minutes_in_between_cards)

# Convert to JSON string for display
json_str = json.dumps(json_object, indent=4)
# print(json_str)

# Specify the file name
file_name = "./dataFiles/seat9.json"

# Write the JSON string to the file
with open(file_name, "w") as file:
    file.write(json_str)

print(f"JSON data has been written to '{file_name}'")
