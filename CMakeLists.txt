build_lib(
    LIBNAME aqua-sim-ng
    SOURCE_FILES
        model/aqua-sim-address.cc
        model/aqua-sim-pt-tag.cc
        model/aqua-sim-channel.cc
        model/aqua-sim-energy-model.cc
        model/aqua-sim-hash-table.cc
        model/aqua-sim-header.cc
        model/aqua-sim-header-goal.cc
        model/aqua-sim-header-mac.cc
        model/aqua-sim-mac.cc
        model/aqua-sim-mobility-pattern.cc
        model/aqua-sim-modulation.cc
        model/aqua-sim-net-device.cc
        model/aqua-sim-node.cc
        model/aqua-sim-noise-generator.cc
        model/aqua-sim-phy.cc
        model/aqua-sim-phy-cmn.cc
        model/aqua-sim-propagation.cc
        model/aqua-sim-range-propagation.cc
        model/aqua-sim-simple-propagation.cc
        model/aqua-sim-routing.cc
        model/aqua-sim-signal-cache.cc
        # model/aqua-sim-sink.cc
        model/aqua-sim-sinr-checker.cc
        helper/aqua-sim-helper.cc
        model/aqua-sim-mac-broadcast.cc
        model/aqua-sim-mac-fama.cc
        model/aqua-sim-mac-aloha.cc
        model/aqua-sim-mac-copemac.cc
        model/aqua-sim-mac-goal.cc
        model/aqua-sim-mac-sfama.cc
        model/aqua-sim-mac-uwan.cc
        model/aqua-sim-rmac.cc
        model/aqua-sim-rmac-buffer.cc
        model/aqua-sim-tmac.cc
        model/aqua-sim-routing-static.cc
        model/aqua-sim-header-routing.cc
        model/aqua-sim-routing-dynamic.cc
        model/aqua-sim-routing-flooding.cc
        model/aqua-sim-routing-buffer.cc
        model/aqua-sim-routing-vbf.cc
        model/aqua-sim-routing-dbr.cc
        model/aqua-sim-routing-vbva.cc
        model/aqua-sim-mobility-kinematic.cc
        model/aqua-sim-mobility-rwp.cc
        model/aqua-sim-synchronization.cc
        model/aqua-sim-localization.cc
        model/aqua-sim-routing-ddos.cc
        model/aqua-sim-attack-model.cc
        model/aqua-sim-trace-reader.cc
        model/ndn/named-data.cc
        model/ndn/named-data-header.cc
        model/ndn/name-discovery.cc
        model/ndn/pit.cc
        model/ndn/fib.cc
        model/ndn/content-storage.cc
        model/ndn/cs-fifo.cc
        model/ndn/cs-lru.cc
        model/ndn/cs-random.cc
        model/ndn/onoff-nd-application.cc
        helper/named-data-helper.cc
        helper/on-off-nd-helper.cc
        helper/aqua-sim-traffic-gen-helper.cc
        model/aqua-sim-traffic-gen.cc
        model/aqua-sim-routing-dummy.cc
        model/aqua-sim-routing-ddbr.cc
        model/lib/svm.cpp
    HEADER_FILES
        model/aqua-sim-address.h
        model/aqua-sim-pt-tag.h
        model/aqua-sim-channel.h
        model/aqua-sim-energy-model.h
        model/aqua-sim-hash-table.h
        model/aqua-sim-header.h
        model/aqua-sim-header-goal.h
        model/aqua-sim-header-mac.h
        model/aqua-sim-mac.h
        model/aqua-sim-mobility-pattern.h
        model/aqua-sim-modulation.h
        model/aqua-sim-net-device.h
        model/aqua-sim-node.h
        model/aqua-sim-noise-generator.h
        model/aqua-sim-phy.h
        model/aqua-sim-phy-cmn.h
        model/aqua-sim-propagation.h
        model/aqua-sim-range-propagation.h
        model/aqua-sim-simple-propagation.h
        model/aqua-sim-routing.h
        model/aqua-sim-signal-cache.h
        # model/aqua-sim-sink.h
        model/aqua-sim-sinr-checker.h
        helper/aqua-sim-helper.h
        model/aqua-sim-mac-broadcast.h
        model/aqua-sim-mac-fama.h
        model/aqua-sim-mac-aloha.h
        model/aqua-sim-mac-copemac.h
        model/aqua-sim-mac-goal.h
        model/aqua-sim-mac-sfama.h
        model/aqua-sim-mac-uwan.h
        model/aqua-sim-rmac.h
        model/aqua-sim-rmac-buffer.h
        model/aqua-sim-tmac.h
        model/aqua-sim-routing-static.h
        model/aqua-sim-header-routing.h
        model/aqua-sim-routing-dynamic.h
        model/aqua-sim-routing-flooding.h
        model/aqua-sim-datastructure.h
        model/aqua-sim-routing-buffer.h
        model/aqua-sim-routing-vbf.h
        model/aqua-sim-routing-dbr.h
        model/aqua-sim-routing-vbva.h
        model/aqua-sim-mobility-kinematic.h
        model/aqua-sim-mobility-rwp.h
        model/aqua-sim-synchronization.h
        model/aqua-sim-localization.h
        model/aqua-sim-routing-ddos.h
        model/aqua-sim-attack-model.h
        model/aqua-sim-trace-reader.h
        model/ndn/named-data.h
        model/ndn/named-data-header.h
        model/ndn/name-discovery.h
        model/ndn/pit.h
        model/ndn/fib.h
        model/ndn/content-storage.h
        model/ndn/cs-fifo.h
        model/ndn/cs-lru.h
        model/ndn/cs-random.h
        model/ndn/onoff-nd-application.h
        helper/named-data-helper.h
        helper/on-off-nd-helper.h
        helper/aqua-sim-traffic-gen-helper.h
        model/aqua-sim-traffic-gen.h
        model/aqua-sim-routing-dummy.h
        model/aqua-sim-routing-ddbr.h
        model/lib/svm.h
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libinternet}
#    TEST_SOURCES test/aqua-sim-test-suite.cc
)

build_lib_example(
    NAME broadcastMAC_example 
    SOURCE_FILES examples/broadcastMAC_example.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME floodtest
    SOURCE_FILES examples/floodtest.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME GOAL_string
    SOURCE_FILES examples/GOAL_string.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME ddos
    SOURCE_FILES examples/ddos.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME bMAC
    SOURCE_FILES examples/bMAC.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libnetanim}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME ND_example
    SOURCE_FILES examples/named_data_example.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME VBF
    SOURCE_FILES examples/VBF.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

build_lib_example(
    NAME FloodingMac
    SOURCE_FILES examples/floodMac.cc
    LIBRARIES_TO_LINK ${libnetwork}
                      ${libenergy}
                      ${libmobility}
                      ${libapplications}
                      ${libaqua-sim-ng}
)

