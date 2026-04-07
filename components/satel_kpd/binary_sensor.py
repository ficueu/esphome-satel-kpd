import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from esphome.const import CONF_ID
from . import satel_kpd_ns, SatelKPD

DEPENDENCIES = ['satel_kpd']

CONF_SATEL_KPD_ID = 'satel_kpd_id'

NAMED_SENSORS = {
    'armed_a': 'set_armed_a_sensor',
    'armed_b': 'set_armed_b_sensor',
    'alarm_a': 'set_alarm_a_sensor',
    'alarm_b': 'set_alarm_b_sensor',
    'trouble': 'set_trouble_sensor',
    'buzzer': 'set_buzzer_sensor',
    'phone': 'set_phone_sensor',
    'power': 'set_power_sensor',
    'input_1': 'set_input_1_sensor',
    'input_2': 'set_input_2_sensor',
    'input_3': 'set_input_3_sensor',
    'input_4': 'set_input_4_sensor',
    'input_5': 'set_input_5_sensor',
    'input_6': 'set_input_6_sensor',
    'input_7': 'set_input_7_sensor',
    'input_8': 'set_input_8_sensor',
    'input_9': 'set_input_9_sensor',
    'input_10': 'set_input_10_sensor',
    'input_11': 'set_input_11_sensor',
    'input_12': 'set_input_12_sensor',
}

schema_dict = {
    cv.GenerateID(CONF_SATEL_KPD_ID): cv.use_id(SatelKPD),
}

for i in range(32):
    schema_dict[cv.Optional(f"bit_{i}")] = binary_sensor.binary_sensor_schema()

for name in NAMED_SENSORS.keys():
    schema_dict[cv.Optional(name)] = binary_sensor.binary_sensor_schema()

CONFIG_SCHEMA = cv.Schema(schema_dict)

async def to_code(config):
    hub = await cg.get_variable(config[CONF_SATEL_KPD_ID])

    for i in range(32):
        key = f"bit_{i}"
        if key in config:
            sens = await binary_sensor.new_binary_sensor(config[key])
            cg.add(hub.set_bit_sensor(i, sens))

    for name, setter in NAMED_SENSORS.items():
        if name in config:
            sens = await binary_sensor.new_binary_sensor(config[name])
            cg.add(getattr(hub, setter)(sens))