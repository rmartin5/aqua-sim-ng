# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def options(opt):
     pass

def configure(conf):
     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('aqua-sim-ng', ['network', 'energy', 'mobility'])
    module.source = [
        'model/aqua-sim-address.cc',
        'model/aqua-sim-pt-tag.cc',
        'model/aqua-sim-channel.cc',
        'model/aqua-sim-energy-model.cc',
        'model/aqua-sim-hash-table.cc',
        'model/aqua-sim-header.cc',
        'model/aqua-sim-header-goal.cc',
        'model/aqua-sim-mac.cc',
       #    'model/aqua-sim-mobility-pattern.cc',
        'model/aqua-sim-modulation.cc',
        'model/aqua-sim-net-device.cc',
        'model/aqua-sim-node.cc',
        'model/aqua-sim-noise-generator.cc',
        'model/aqua-sim-phy.cc',
        'model/aqua-sim-phy-cmn.cc',
        'model/aqua-sim-propagation.cc',
        'model/aqua-sim-simple-propagation.cc',
        'model/aqua-sim-routing.cc',
        'model/aqua-sim-signal-cache.cc',
       # 'model/aqua-sim-sink.cc',
        'model/aqua-sim-sinr-checker.cc',
        'helper/aqua-sim-helper.cc',
        'model/aqua-sim-mac-fama.cc',
        'model/aqua-sim-mac-aloha.cc',
        'model/aqua-sim-mac-copemac.cc',
        'model/aqua-sim-mac-goal.cc',
        'model/aqua-sim-mac-sfama.cc',
        'model/aqua-sim-mac-uwan.cc',
        'model/aqua-sim-rmac.cc',
        'model/aqua-sim-rmac-buffer.cc',
        'model/aqua-sim-tmac.cc',
        'model/aqua-sim-routing-static.cc',
        ]

    module_test = bld.create_ns3_module_test_library('aqua-sim-ng')
    module_test.source = [
        # 'test/aqua-sim-test-suite.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'aqua-sim-ng'
    headers.source = [
        'model/aqua-sim-address.h',
        'model/aqua-sim-pt-tag.h',
        'model/aqua-sim-channel.h',
        'model/aqua-sim-energy-model.h',
        'model/aqua-sim-hash-table.h',
        'model/aqua-sim-header.h',
        'model/aqua-sim-header-goal.h',
        'model/aqua-sim-mac.h',
       #    'model/aqua-sim-mobility-pattern.h',
        'model/aqua-sim-modulation.h',
        'model/aqua-sim-net-device.h',
        'model/aqua-sim-node.h',
        'model/aqua-sim-noise-generator.h',
        'model/aqua-sim-phy.h',
        'model/aqua-sim-phy-cmn.h',
        'model/aqua-sim-propagation.h',
        'model/aqua-sim-range-propagation.h',
        'model/aqua-sim-simple-propagation.h',
        'model/aqua-sim-routing.h',
        'model/aqua-sim-signal-cache.h',
       # 'model/aqua-sim-sink.h',
        'model/aqua-sim-sinr-checker.h',
        'helper/aqua-sim-helper.h',
        'model/aqua-sim-mac-fama.h',
        'model/aqua-sim-mac-aloha.h',
        'model/aqua-sim-mac-copemac.h',
        'model/aqua-sim-mac-goal.h',
        'model/aqua-sim-mac-sfama.h',
        'model/aqua-sim-mac-uwan.h',
        'model/aqua-sim-rmac.h',
        'model/aqua-sim-rmac-buffer.h',
        'model/aqua-sim-tmac.h',
        'model/aqua-sim-routing-static.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    bld.ns3_python_bindings()
