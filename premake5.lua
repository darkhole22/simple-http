workspace "SimpleHTTP"
    architecture "x64"
    configurations { "Debug", "Release" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

include ("SimpleHTTP/SimpleHTTP")

include ("SimpleHTTPTestbed/SimpleHTTPTestbed")

