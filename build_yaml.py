import struct, yaml, sys

def read_pdb(filename):
    with open(filename, "rb") as f:
        data = f.read()
    return data

def parse_pdb(data):
    num_records = struct.unpack_from(">H", data, 76)[0]
    offset = 78
    records = []

    record_headers = [] # builds record list
    for i in range(num_records):
        rec_offset, attr, uid1, uid2, uid3 = struct.unpack_from(">LBBBB", data, offset)
        uid = (uid1 << 16) | (uid2 << 8) | uid3
        record_headers.append((rec_offset, uid))
        offset += 8

    record_data = {}
    for i in range(num_records):
        start = record_headers[i][0]
        end = record_headers[i+1][0] if i < num_records - 1 else len(data)
        record_data[record_headers[i][1]] = data[start:end]

    return record_data

def unpack_recipe(recipe_record, ingredient_records, unit_records):
    offset = 0
    name, num_ing = struct.unpack_from(">32sB", recipe_record, offset)
    offset += 34
    name = name.split(b"\x00")[0].decode("ascii")

    recipe_quants = list(struct.unpack_from(f">{num_ing}B", recipe_record, offset))
    offset += num_ing
    recipe_fracs = list(struct.unpack_from(f">{num_ing}B", recipe_record, offset))
    offset += num_ing
    recipe_denoms = list(struct.unpack_from(f">{num_ing}B", recipe_record, offset))
    offset += num_ing

    ingredient_ids = list(struct.unpack_from(f">{num_ing}L", recipe_record, offset))
    offset += 4 * num_ing

    unit_ids = list(struct.unpack_from(f">{num_ing}L", recipe_record, offset))
    
    offset += 4 * num_ing

    # assumes steps are remaining bytes, null-terminated
    steps = recipe_record[offset:].split(b"\x00", 1)[0].decode("ascii", "ignore")
    
    ingredients = []
    
    for i in range(num_ing):
        ingredients.append( {
            "name": ingredient_records[ingredient_ids[i]].decode("ascii", "ignore").rstrip("\0"),
            "unit": unit_records[unit_ids[i]].decode("ascii", "ignore").rstrip("\0"),
            "whole": recipe_quants[i],
            "frac": recipe_fracs[i],
            "denom": recipe_denoms[i]
        } )

    return {
        "name": name,
        "ingredients": ingredients,
        "steps": steps
    }

if __name__ == "__main__":
    if len(sys.argv) != 5:
        print("Usage: python3 pdb_to_yaml.py [Recipe PDB] [Ingredients PDB] [Units PDB] [Output YML]")
        sys.exit(1)
    recipe_db = parse_pdb(read_pdb(sys.argv[1]))
    ingredients_db = parse_pdb(read_pdb(sys.argv[2]))
    units_db = parse_pdb(read_pdb(sys.argv[3])) 
    recipes = {"recipes": [unpack_recipe(v, ingredients_db, units_db) for v in recipe_db.values()]}
    
    with open(sys.argv[4], "w") as f:
        yaml.dump(recipes, f, sort_keys=False)
        print(f"Wrote {sys.argv[4]} ({len(recipes)} records)")