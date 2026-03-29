import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID
from . import satel_kpd_ns, SatelKPD

DEPENDENCIES = ['satel_kpd']

CONF_SATEL_KPD_ID = 'satel_kpd_id'
CONF_TROUBLE_TEXT = 'trouble_text'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_SATEL_KPD_ID): cv.use_id(SatelKPD),
    cv.Optional(CONF_TROUBLE_TEXT): text_sensor.text_sensor_schema(),
})

async def to_code(config):
    hub = await cg.get_variable(config[CONF_SATEL_KPD_ID])
    if CONF_TROUBLE_TEXT in config:
        sens = await text_sensor.new_text_sensor(config[CONF_TROUBLE_TEXT])
        cg.add(hub.set_trouble_text_sensor(sens))