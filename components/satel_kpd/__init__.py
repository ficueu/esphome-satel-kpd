import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

__version__ = "1.1"

AUTO_LOAD = ['binary_sensor', 'text_sensor']

satel_kpd_ns = cg.esphome_ns.namespace('satel_kpd')
SatelKPD = satel_kpd_ns.class_('SatelKPD', cg.Component)

CONF_DATA_PIN = 'data_pin'
CONF_CKL_PIN = 'ckl_pin'
CONF_PRS_PIN = 'prs_pin'
CONF_SIMULATED_KEYPAD = 'simulated_keypad'
CONF_TROUBLE_LANG = 'trouble_lang'
CONF_VARIANT = 'variant'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(SatelKPD),
    cv.Required(CONF_DATA_PIN): pins.internal_gpio_input_pin_schema,
    cv.Required(CONF_CKL_PIN): pins.internal_gpio_input_pin_schema,
    cv.Optional(CONF_PRS_PIN): pins.internal_gpio_output_pin_schema,
    cv.Optional(CONF_SIMULATED_KEYPAD, default=False): cv.boolean,
    cv.Optional(CONF_TROUBLE_LANG, default='en'): cv.one_of('en', 'pl', lower=True),
    cv.Optional(CONF_VARIANT, default='ca6'): cv.one_of('ca6', 'ca10', lower=True),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    data_pin = await cg.gpio_pin_expression(config[CONF_DATA_PIN])
    cg.add(var.set_data_pin(data_pin))

    ckl_pin = await cg.gpio_pin_expression(config[CONF_CKL_PIN])
    cg.add(var.set_ckl_pin(ckl_pin))

    if CONF_PRS_PIN in config:
        prs_pin = await cg.gpio_pin_expression(config[CONF_PRS_PIN])
        cg.add(var.set_prs_pin(prs_pin))
    
    cg.add(var.set_simulated_keypad(config[CONF_SIMULATED_KEYPAD]))
    cg.add(var.set_trouble_lang(config[CONF_TROUBLE_LANG]))
    cg.add(var.set_variant(config[CONF_VARIANT]))