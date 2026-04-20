import json

# Configuration: How much to subtract from current values
SHIFT_X = 25
SHIFT_Y = 12

input_filename = "resources/scenes/homework3_map.scene"
output_filename = "resources/scenes/scene_shifted.scene"

try:
    with open(input_filename, 'r') as f:
        data = json.load(f)

    # Apply the shift
    for actor in data.get("actors", []):
        if "x" in actor:
            actor["x"] -= SHIFT_X
        if "y" in actor:
            actor["y"] -= SHIFT_Y

    # Save the new file
    with open(output_filename, 'w') as f:
        json.dump(data, f, indent=4)
        
    print(f"Success! Shifted coordinates saved to {output_filename}")
    print(f"Origin (0,0) is now at (-{SHIFT_X}, -{SHIFT_Y})")

except FileNotFoundError:
    print(f"Error: Could not find {input_filename}")