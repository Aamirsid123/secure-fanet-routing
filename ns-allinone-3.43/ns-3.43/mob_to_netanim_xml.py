import re
import xml.etree.ElementTree as ET

def parse_mob_file(filepath):
    node_movements = {}
    with open(filepath, 'r') as file:
        for line in file:
            match = re.search(r'now=\+(\d+)ns node=(\d+) pos=([\d\.\-]+):([\d\.\-]+):([\d\.\-]+)', line)
            if match:
                time_ns = int(match.group(1))  # in nanoseconds
                node_id = int(match.group(2))
                pos_x = float(match.group(3))
                pos_y = float(match.group(4))
                pos_z = float(match.group(5))
                if node_id not in node_movements:
                    node_movements[node_id] = []
                node_movements[node_id].append((time_ns, pos_x, pos_y, pos_z))
    return node_movements

def generate_netanim_xml(node_movements, output_filename):
    # Create XML structure
    root = ET.Element('anim', version="3.0")

    # Set Background size (optional)
    ET.SubElement(root, 'topology', xmin="0", ymin="0", xmax="1500", ymax="1500")

    # Add nodes
    for node_id in node_movements:
        ET.SubElement(root, 'node', id=str(node_id), locX="0", locY="0", locZ="0")

    # Add location updates
    for node_id, movements in node_movements.items():
        for movement in movements:
            time_ns, x, y, z = movement
            ET.SubElement(root, 'location', id=str(node_id),
                          time=str(time_ns // 1000000),   # Convert ns ➔ ms (NetAnim uses milliseconds)
                          x=str(x), y=str(y), z=str(z))

    # Write to file
    tree = ET.ElementTree(root)
    tree.write(output_filename, encoding='utf-8', xml_declaration=True)
    print(f"\n✅ Generated NetAnim XML: {output_filename}")

# Usage
input_mob_file = '/Users/aamirsiddiqui/Desktop/Project/ns-allinone-3.43/ns-3.43/AODV-secure-manet-routing-output.mob'
# change if needed
output_xml_file = 'AODV-netanim.xml'

node_movements = parse_mob_file(input_mob_file)
generate_netanim_xml(node_movements, output_xml_file)
