#!/bin/bash 

find_vendor=$1
find_product=$2

for udi in `hal-find-by-capability --capability serial | sort` 
do 
  parent=`hal-get-property --udi ${udi} --key "info.parent"` 
  device=`hal-get-property --udi ${udi} --key "linux.device_file"` 
  vendor=`hal-get-property --udi ${parent} --key "usb.vendor_id"` 
  product=`hal-get-property --udi ${parent} --key "usb.product_id"` 
  driver=`hal-get-property --udi ${parent} --key "info.linux.driver"` 
  bus=`hal-get-property --udi ${parent} --key "usb.bus_number"` 
  interf=`hal-get-property --udi ${parent} --key "usb.interface.number"` 
  grandpa=`hal-get-property --udi ${parent} --key "info.parent"` 
  name=`hal-get-property --udi ${grandpa} --key "info.product"` 
  serial=`hal-get-property --udi ${grandpa} --key "usb_device.serial"` 

  # convert to hex
  vendor=$(printf "%.4x" "$vendor")
  product=$(printf "%.4x" "$product")

  if [ "$vendor" = "$find_vendor" ]
  then
    if [ "$product" = "$find_product" ]
    then
      #printf "%s:%.4x:%.4x:%d:%s:%s:%s:%s\n" "${device}" "${vendor}" "${product}" "${interf}" "${driver}" "${bus}" "${serial}" "${name}" 
      echo "${device}"
    fi
  fi
done 
