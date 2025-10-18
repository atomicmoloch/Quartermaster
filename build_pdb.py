import struct, yaml, sys
from datetime import datetime

class PalmRecord:
    def __init__(self, data: bytes, uid: int):
        self.data = data
        self.unique_id = uid  # 24-bit integer

def read_data(path):
    with open(path) as f:
        return yaml.safe_load(f)
    # Possibly add json support

def palm_timestamp():
    # Seconds since Jan 1, 1904
    epoch_1904 = datetime(1904, 1, 1)
    now = datetime.now()
    seconds = int((now - epoch_1904).total_seconds())
    return seconds & 0xFFFFFFFF  # I hate silent promotion

def write_pdb(filename, dbname, creator, typecode, records):
    num_records = len(records)
    header = struct.pack(
        ">32s HH LLL LLL 4s4s LLH",
        dbname.encode("ascii").ljust(32, b"\x00"), # database name (32)
        0,  # flags (2)
        0,  # version (2)
        palm_timestamp(), # create time (4)
        palm_timestamp(), # modified time (4)
        palm_timestamp(), # backup time (4)
        0,  # modified number (4)
        0,  # app info size (4)
        0,  # sort info size (4)
        typecode.encode("ascii")[:4], # file type (4)
        creator.encode("ascii")[:4], # creator id (4)
        0,  # unique id seed (4)
        0, # next record number (4)
        num_records, # number of records (2)
    )

    offset = 78 + num_records * 8 # header + record list
    record_headers = b""
    body = b""
    for i, rec in enumerate(records):
        uid1 = (rec.unique_id >> 16) & 0xFF
        uid2 = (rec.unique_id >> 8) & 0xFF
        uid3 = rec.unique_id & 0xFF

        record_headers += struct.pack(">LBBBB", offset, 0, uid1, uid2, uid3)
        body += rec.data
        offset += len(rec.data)
    with open(filename, "wb") as f:
        f.write(header)
        f.write(record_headers)
        f.write(body)
    print(f"Wrote {filename} ({num_records} records)")
    
    
def build_records(data):
    recipe_records = []
    next_recipe_id = 1

    ingredient_records = []
    unit_records = []

    ingredient_ids = {} #lookup dict for ingredient ids
    next_ingredient_id = 1

    unit_ids = {} #lookup dict for unit ids
    next_unit_id = 1
    
    for r in data["recipes"]:
        name = r["name"].encode("ascii", errors='ignore')[:31] + b"\x00"
        steps = r.get("steps", "").replace("\\n", "\n").encode("ascii", errors='ignore') + b"\x00"
        # Allows steps to be omitted

        num_ing = len(r["ingredients"])  # max 256

        ingredient_names = [x["name"] for x in r["ingredients"]]
        unit_names = [x["unit"] for x in r["ingredients"]]

        quantities = [x["whole"] for x in r["ingredients"]]
        fracs = [x["frac"] for x in r["ingredients"]]
        denoms = [x["denom"] for x in r["ingredients"]]

        recipe_ingredient_ids = []
        recipe_unit_ids = []

        for i, ingredient in enumerate(ingredient_names):
            if ingredient in ingredient_ids:
                recipe_ingredient_ids.append(ingredient_ids[ingredient])
            else:
                ingredient_ids[ingredient] = next_ingredient_id
                recipe_ingredient_ids.append(next_ingredient_id)
                next_ingredient_id += 1

        for i, unit in enumerate(unit_names):
            if unit in unit_ids:
                recipe_unit_ids.append(unit_ids[unit])
            else:
                unit_ids[unit] = next_unit_id
                recipe_unit_ids.append(next_unit_id)
                next_unit_id += 1

        record = struct.pack(">32sBB", name, num_ing, 0)
        record += struct.pack(f">{num_ing}B", *quantities)
        record += struct.pack(f">{num_ing}B", *fracs)
        record += struct.pack(f">{num_ing}B", *denoms)
        record += struct.pack(f">{num_ing}L", *recipe_ingredient_ids)
        record += struct.pack(f">{num_ing}L", *recipe_unit_ids)
        record += struct.pack(f">{len(steps)}s", steps)

        recipe_records.append(PalmRecord(record, next_recipe_id))
        next_recipe_id += 1
        
        
    ingredient_ids = dict(sorted(ingredient_ids.items()))
    unit_ids = dict(sorted(unit_ids.items()))

    for key in ingredient_ids:
        data = key.encode("ascii") + b"\x00"
        ingredient_records.append(PalmRecord(data, ingredient_ids[key]))

    for key in unit_ids:
        data = key.encode("ascii") + b"\x00"
        unit_records.append(PalmRecord(data, unit_ids[key]))

    return unit_records, ingredient_records, recipe_records

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: build_pdb.py [yaml recipe file]")
        sys.exit(1)
    data = read_data(sys.argv[1])
    unit_recs, ing_recs, recipe_recs = build_records(data)
    write_pdb(f"Units{palm_timestamp()}.pdb", "QMUnits", "WOEM", "Data", unit_recs)
    write_pdb(f"Ingredients{palm_timestamp()}.pdb", "QMIngredients", "WOEM", "Data", ing_recs)
    write_pdb(f"Recipes{palm_timestamp()}.pdb", "QMRecipes", "WOEM", "Data", recipe_recs)
