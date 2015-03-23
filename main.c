#include <stdio.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
#include <IOKit/hid/IOHIDLib.h>

const int vendor_id = 0x077d;
const int product_id = 0x0410;

typedef struct {
    IOHIDElementRef button;
    IOHIDElementRef scroll;
    IOHIDElementRef led;
} PowerMateElements_t;

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

static void DeviceAttached(void *inContext, IOReturn inResult, void *inSender,
        IOHIDDeviceRef inIOHIDDeviceRef) {
    printf("Device attached\n");
}

static void DeviceRemoved(void *inContext, IOReturn inResult, void *inSender,
        IOHIDDeviceRef inIOHIDDeviceRef) {
    printf("Device removed\n");
}

static void ValueAvailable(void *inContext, IOReturn inResult, void *inSender) {
    IOHIDQueueRef queue = (IOHIDQueueRef)inSender;
    PowerMateElements_t *elements = (PowerMateElements_t *)inContext;

    do {
        IOHIDValueRef value = IOHIDQueueCopyNextValueWithTimeout(queue, 0.0);
        if (!value) break;

        CFIndex int_value = IOHIDValueGetIntegerValue(value);
        IOHIDElementRef element = IOHIDValueGetElement(value);

        if (element == elements->button) {
            printf("Button press: %ld\n", int_value);
        } else if (element == elements->scroll) {
            printf("Scroll: %ld\n", int_value);
        } else {
            printf("Unknown element\n");
        }

        CFRelease(value);
    } while (true);
}

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

    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, DeviceAttached, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, DeviceRemoved, NULL);
    IOHIDManagerScheduleWithRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    if (IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone) != kIOReturnSuccess) {
        printf("Error opening HID manager\n");
        return 1;
    }

    CFSetRef devices = IOHIDManagerCopyDevices(hid_manager);

    if (!devices) {
        printf("No PowerMate devices found\n");
        return 1;
    }

    CFIndex deviceCount = CFSetGetCount(devices);
    printf("Device count: %ld\n", deviceCount);

    IOHIDDeviceRef *device_refs = malloc(sizeof(IOHIDDeviceRef) * deviceCount);
    CFSetGetValues(devices, (const void **)device_refs);

    IOHIDDeviceRef device = device_refs[0];

    IOHIDQueueRef queue = IOHIDQueueCreate(kCFAllocatorDefault, device, 100, kIOHIDOptionsTypeNone);

    CFArrayRef all_elements = IOHIDDeviceCopyMatchingElements(device, NULL, kIOHIDOptionsTypeNone);
    CFIndex elementCount = CFArrayGetCount(all_elements);

    if (elementCount < 9) {
        printf("Too few elements found: %ld\n", elementCount);
        return 1;
    }

    PowerMateElements_t elements;
    elements.button = (IOHIDElementRef)CFArrayGetValueAtIndex(all_elements, 1);
    elements.scroll = (IOHIDElementRef)CFArrayGetValueAtIndex(all_elements, 2);
    elements.led = (IOHIDElementRef)CFArrayGetValueAtIndex(all_elements, 8);

    IOHIDQueueRegisterValueAvailableCallback(queue, ValueAvailable, &elements);
    IOHIDQueueAddElement(queue, elements.button);
    IOHIDQueueAddElement(queue, elements.scroll);
    IOHIDQueueScheduleWithRunLoop(queue, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDQueueStart(queue);
    CFRunLoopRun();

    return 0;
}

