# See https://git.yoctoproject.org/poky/tree/meta/files/common-licenses
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# TODO: Set this  with the path to your assignments rep.  Use ssh protocol and see lecture notes
# about how to setup ssh-agent for passwordless access
# SRC_URI = "git://git@github.com/cu-ecen-aeld/<your assignments repo>;protocol=ssh;branch=master"
SRC_URI = "git://github.com/cu-ecen-aeld/assignments-3-and-later-IsaacGH17.git;protocol=https;branch=main"
PV = "1.0+git${SRCPV}"
# TODO: set to reference a specific commit hash in your assignment repo
#SRCREV = "f99b82a5d4cb2a22810104f89d4126f52f4dfaba"
SRCREV = "ba5c2b97dfc210c324ebe8c865625f66a2de69b3"
# This sets your staging directory based on WORKDIR, where WORKDIR is defined at 
# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-WORKDIR
# We reference the "server" directory here to build from the "server" directory
# in your assignments repo
S = "${WORKDIR}/git/server"

# TODO: Add the aesdsocket application and any other files you need to install
# See https://git.yoctoproject.org/poky/plain/meta/conf/bitbake.conf?h=kirkstone
# Nota: NO declaramos virtual/${TARGET_PREFIX}gcc en DEPENDS — en wrynose
# ese virtual ya no existe como provider nombrado y bitbake falla con
# "Nothing PROVIDES 'virtual/aarch64-poky-linux-gcc'". La clase base
# ya agrega la dependencia del toolchain cross implicitamente.

FILES:${PN} += "${bindir}/aesdsocket"
# update-rc.d-native la hereda la clase update-rc.d; no hace falta declararla
inherit update-rc.d
INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME:${PN} = "aesdsocket-start-stop"
INITSCRIPT_PARAMS:${PN} = "defaults 99"
FILES:${PN} += "${sysconfdir}/init.d/aesdsocket-start-stop"
# TODO: customize these as necessary for any libraries you need for your application
# (and remove comment)
TARGET_LDFLAGS += "-pthread -lrt"

do_configure () {
	:
}

do_compile () {
    oe_runmake 'CROSS_COMPILE=${TARGET_PREFIX}' 'CC=${CC}' 'CFLAGS=${CFLAGS}' 'LD=${LD}' 'LDFLAGS=${LDFLAGS}'
}

do_install () {
	# TODO: Install your binaries/scripts here.
	# Be sure to install the target directory with install -d first
	# Yocto variables ${D} and ${S} are useful here, which you can read about at 
	# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-D
	# and
	# https://docs.yoctoproject.org/ref-manual/variables.html?highlight=workdir#term-S
	# See example at https://github.com/cu-ecen-aeld/ecen5013-yocto/blob/ecen5013-hello-world/meta-ecen5013/recipes-ecen5013/ecen5013-hello-world/ecen5013-hello-world_git.bb
    install -d ${D}${bindir}
    install -m 0755 ${S}/aesdsocket ${D}${bindir}/aesdsocket
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${S}/aesdsocket-start-stop.sh ${D}${sysconfdir}/init.d/aesdsocket-start-stop
}
