SUMMARY = "Modulos misc-modules de ldd3 (hello + faulty) para el gate assignment7 del autograder"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

inherit module

PV = "1.0+git${SRCPV}"

SRC_URI = "git://github.com/IsaacGH17/aeld-assignment7-isaacgh17.git;protocol=https;branch=main \
           file://misc-modules-objm.patch"
SRCREV = "f3ad505040c2bd87114a771e68f71cd9d278b440"

S = "${UNPACKDIR}/${BP}/misc-modules"

# El Makefile usa KERNELDIR (= /lib/modules/$(uname -r)/build por default,
# que apunta al kernel del HOST). El target 'modules' compila TODOS los
# obj-m del dir (hello, hellop, seq, jiq, sleepy, complete, silly, faulty,
# kdatasize, kdataalign, jit); solo instalamos hello.ko y faulty.ko.
EXTRA_OEMAKE += "KERNELDIR=${STAGING_KERNEL_BUILDDIR}"

# El Makefile pone LDDINC=$(PWD)/../include, pero dentro de kbuild PWD
# apunta al dir del kernel y el -I se pierde; ademas el kernel 6.18 ya no
# procesa EXTRA_CFLAGS de los Makefiles externos (los .c dan "fatal error:
# proc_ops_version.h / access_ok_version.h not found" sin ningun -I en la
# linea de gcc). KCFLAGS si llega a todos los cc de kbuild via MAKEFLAGS.
# scull/main.c pide access_ok_version.h y pipe.c proc_ops_version.h; en
# misc-modules lo piden seq.c, jiq.c y jit.c (hello.c y faulty.c no).

EXTRA_OEMAKE += "KCFLAGS=-I${S}/../include"

MAKE_TARGETS = "modules"

KERNEL_MODULE_AUTOLOAD += "hello faulty"

# La clase module corre 'make modules_install' en do_install y el target
# existe, pero instala TODO el obj-m; solo hello y faulty son necesarios,
# asi que instalamos a mano los dos .ko en el arbol que kernel-module-split
# escanea para generar los paquetes y los .conf de autoload.
do_install () {
	install -d ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra
	install -m 0644 ${S}/hello.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/hello.ko
	install -m 0644 ${S}/faulty.ko ${D}${nonarch_base_libdir}/modules/${KERNEL_VERSION}/extra/faulty.ko
}
