import numpy as np


def gen_schema(file, num_values_max=1024, value_max=np.iinfo(np.uint64).max, num_values_min=1, value_min=0):
    import pyarrow as pa

    output_schema = pa.schema([
        pa.field("voltage", pa.list_(
            pa.field("item", pa.uint64(), False).with_metadata(
                {"illex_MIN": str(value_min), "illex_MAX": str(value_max)})
        ), False).with_metadata(
            {"illex_MIN_LENGTH": str(num_values_min), "illex_MAX_LENGTH": str(num_values_max)}
        )
    ])

    pa.output_stream(file).write(output_schema.serialize())
