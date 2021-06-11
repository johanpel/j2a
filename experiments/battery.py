import pyarrow as pa

output_schema = pa.schema([
    pa.field("voltage", pa.list_(
        pa.field("item", pa.uint64(), False).with_metadata(
            {"illex_MIN": "0", "illex_MAX": "2047"})
    ), False).with_metadata(
        {"illex_MIN_LENGTH": "1", "illex_MAX_LENGTH": "16"}
    )
])

pa.output_stream("battery.as").write(output_schema.serialize())

print("Battery Status schema generated.")