inherit core-image
CORE_IMAGE_EXTRA_INSTALL += "openssh aesd-assignments aesd-char-driver kernel-module-aesdchar"
# Modulos ldd3 del assignment 7: el autograder validate_assignment7_qemu
# exige hello/faulty/scull cargados al boot (los paquetes kernel-module-*
# llevan el .ko + el conf de /etc/modules-load.d).
CORE_IMAGE_EXTRA_INSTALL += "aesd-misc-modules aesd-scull kernel-module-hello kernel-module-faulty kernel-module-scull"
inherit extrausers
# See https://docs.yoctoproject.org/singleindex.html#extrausers-bbclass
# We set a default password of root to match our busybox instance setup
# Don't do this in a production image
# PASSWD below is set to the output of
# printf "%q" $(mkpasswd -m sha256crypt root) to hash the "root" password
# string
PASSWD = "\$5\$2WoxjAdaC2\$l4aj6Is.EWkD72Vt.byhM5qRtF9HcCM/5YpbxpmvNB5"
EXTRA_USERS_PARAMS = "usermod -p '${PASSWD}' root;"
# Allow root user to login to the image
EXTRA_IMAGE_FEATURES:append = " allow-root-login"
# Build an ext4 image for ease of use with runqemu
IMAGE_FSTYPES:append = " ext4"

#TODO: remove comment here to add to your image
#CORE_IMAGE_EXTRA_INSTALL += "aesd-assignments"
