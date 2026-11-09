SUMMARY = "AESD char driver (asignacion 8)"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module

SRC_URI = "git://github.com/cu-ecen-aeld/assignments-3-and-later-IsaacGH17.git;protocol=https;branch=main"
SRCREV = "ba5c2b97dfc210c324ebe8c865625f66a2de69b3"

S = "${UNPACKDIR}/${BP}/aesd-char-driver"

# La clase module compila contra el kernel de la imagen. El Makefile del
# driver usa KERNELDIR, no KERNEL_SRC, asi que hay que pasarle el path
# del build dir del kernel target (el default del Makefile apunta al
# kernel del HOST y produce un .ko con vermagic invalido en el target).
EXTRA_OEMAKE += "KERNELDIR=${STAGING_KERNEL_BUILDDIR}"

MAKE_TARGETS = "modules"

# El autograder exige 'lsmod | grep aesdchar' al arranque: cargar el
# modulo automaticamente en boot.
KERNEL_MODULE_AUTOLOAD += "aesdchar"

# La clase module corre 'make modules_install' en do_install, pero el
# Makefile del driver solo define el target 'modules' (no modules_install),
# asi que oe_runmake falla con "No rule to make target 'modules_install'".
# Instalamos el .ko a mano en el arbol que kernel-module-split escanea
# (${nonarch_base_libdir}/modules/${KERNEL_VERSION}) para que genere el
# paquete del modulo y el .conf de autoload.
do_install () {
	install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra
	install -m 0644 ${S}/aesdchar.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/aesdchar.ko
}
