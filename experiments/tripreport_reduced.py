import numpy as np


def gen_schema(file, value_max=np.iinfo(np.uint32).max, value_min=0):
    import pyarrow as pa

    schema_fields = [pa.field("timestamp", pa.utf8(), False),
                     pa.field("timezone", pa.uint32(), False).with_metadata(
                         {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                     pa.field("odometer", pa.uint32(), False).with_metadata(
                         {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                     pa.field("hypermiling", pa.bool_(), False),
                     pa.field("avgspeed", pa.uint32(), False).with_metadata(
                         {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                     pa.field("sec_in_band",
                              pa.list_(pa.field("item", pa.uint32(), False)
                                       .with_metadata({"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                                       12), False),
                     pa.field("braking", pa.list_(
                         pa.field("item", pa.uint32(), False).with_metadata(
                             {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                         6), False),
                     pa.field("accel", pa.list_(
                         pa.field("item", pa.uint32(), False).with_metadata(
                             {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                         6), False),
                     pa.field("orientation", pa.bool_(), False),
                     pa.field("small_speed_var", pa.list_(
                         pa.field("item", pa.uint32(), False).with_metadata(
                             {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                         13), False),
                     pa.field("large_speed_var", pa.list_(
                         pa.field("item", pa.uint32(), False).with_metadata(
                             {"illex_MIN": str(value_min), "illex_MAX": str(value_max)}),
                         13), False)
                     ]

    schema = pa.schema(schema_fields)
    serialized_schema = schema.serialize()
    pa.output_stream(file).write(serialized_schema)
