vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/CommonLibVR
    REF bedcb1e05418baba7b316a650b6180c2dd6007a8
    SHA512 6c9eb136546f816a23bd3a1c010216eaa4383cf8ca9b3a125161dfd5d17c657c258b680d8e3a90437857505a6f2373a8d53d3952a22aa476e8fcad05e8a2fb50
    HEAD_REF ng
)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH2
    REPO ValveSoftware/openvr
    REF 60eb187801956ad277f1cae6680e3a410ee0873b
    SHA512 bb85b4705e7095ac65df9969112b2df8930cee7917cc5f14231c5a0ffeed7a73ffa60727fd32f8786a403656f95a3ec0f80bf3ceabc5b8ede964aefb920bc718
    HEAD_REF master
)

file(COPY "${SOURCE_PATH2}/" DESTINATION "${SOURCE_PATH}/extern/openvr")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF 
        -DSKSE_SUPPORT_XBYAK=ON
        -DSKSE_SUPPORT_PATCH_SAFETY=OFF
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/CommonLibSSE")

file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${SOURCE_PATH2}/headers/openvr.h" DESTINATION ${CURRENT_PACKAGES_DIR}/include)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

if(EXISTS "${SOURCE_PATH}/COPYING") # COPYING = new LICENSE
    vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
endif() 
