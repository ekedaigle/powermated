#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

const int vendor_id = 0x077d;
const int product_id = 0x0410;

// function to get a long device property

// returns FALSE if the property isn't found or can't be converted to a long

static Boolean IOHIDDevice_GetLongProperty(
    IOHIDDeviceRef inDeviceRef,     // the HID device reference
    CFStringRef inKey,              // the kIOHIDDevice key (as a CFString)
    long * outValue)                // address where to return the output value
{
    Boolean result = FALSE;
 
    CFTypeRef tCFTypeRef = IOHIDDeviceGetProperty(inDeviceRef, inKey);
    if (tCFTypeRef) {
        // if this is a number
        if (CFNumberGetTypeID() == CFGetTypeID(tCFTypeRef)) {
            // get its value
            result = CFNumberGetValue((CFNumberRef) tCFTypeRef, kCFNumberSInt32Type, outValue);
        }
    }
    return result;
}   // IOHIDDevice_GetLongProperty

int main(int argc, const char *argv[]) {
	// create a IO HID Manager reference
	IOHIDManagerRef hid_manager = IOHIDManagerCreate( kCFAllocatorDefault, kIOHIDOptionsTypeNone );

    CFMutableDictionaryRef match = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    if (!match) {
        printf("Error creating dictionary\n");
        return 1;
    }

    CFNumberRef vendor_id_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor_id);
    CFNumberRef product_id_ref = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product_id);
    CFDictionarySetValue(match, CFSTR(kIOHIDVendorIDKey), vendor_id_ref);
    CFDictionarySetValue(match, CFSTR(kIOHIDProductIDKey), product_id_ref);
    CFRelease(vendor_id_ref);
    CFRelease(product_id_ref);

    IOHIDManagerSetDeviceMatching(hid_manager, match);
    CFRelease(match);

    if (IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
        printf("Error opening HID manager\n");
        return 1;
    }

    CFSetRef devices = IOHIDManagerCopyDevices(hid_manager);

    if (!devices) {
        printf("Error getting device set\n");
        return 1;
    }

    CFIndex deviceCount = CFSetGetCount(devices);
    printf("Device count: %ld\n", deviceCount);

    IOHIDDeviceRef *device_refs = malloc(sizeof(IOHIDDeviceRef) * deviceCount);
    CFSetGetValues(devices, (const void **)device_refs);

    for (CFIndex deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
        IOHIDDeviceRef device = device_refs[deviceIndex];
        CFArrayRef all_elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
        CFIndex elementCount = CFArrayGetCount(all_elements);

        printf("Element count: %ld\n", elementCount);
        CFIndex break_index = 1;

        for (CFIndex elementIndex = 0; elementIndex < elementCount; elementIndex++) {
            IOHIDElementRef element = (IOHIDElementRef)CFArrayGetValueAtIndex(all_elements, elementIndex);
            uint32_t usagePage = IOHIDElementGetUsagePage(element);
            printf("Usage page: %u, ", usagePage);

            IOHIDElementType element_type = IOHIDElementGetType(element);
            printf("Element type: %d, ", element_type);

            CFIndex min = IOHIDElementGetLogicalMin(element);
            CFIndex max = IOHIDElementGetLogicalMax(element);
            printf("Min: %ld, Max: %ld\n", min, max);

            if (element_type == 257) {
                IOHIDValueRef value_ref = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, 0, 0);
                IOHIDDeviceSetValue(device, element, value_ref);
                break;
            }
        }
    }

    return 0;
}

