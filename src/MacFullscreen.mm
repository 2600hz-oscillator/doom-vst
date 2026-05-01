#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <objc/runtime.h>
#include <juce_gui_basics/juce_gui_basics.h>

// NSWindow's default constrainFrameRect:toScreen: shifts the window down by
// the menu-bar height even at high window levels — so [setFrame: screen.frame]
// silently puts the window's top edge below the menu bar. We isa-swizzle the
// window to a dynamic subclass that returns the input rect unchanged. The
// dynamic class is created lazily and reused across windows of the same
// origin class. Per-instance via object_setClass so other NSWindows in the
// host process are unaffected.
static void dvUnconstrainWindow(NSWindow* nsWindow)
{
    Class origClass = [nsWindow class];
    NSString* newName = [NSString stringWithFormat: @"DV_Unconstrained_%@",
                                                    NSStringFromClass(origClass)];
    Class newClass = objc_getClass([newName UTF8String]);
    if (newClass == nil)
    {
        newClass = objc_allocateClassPair(origClass, [newName UTF8String], 0);
        if (newClass == nil) return;

        SEL sel = @selector(constrainFrameRect:toScreen:);
        Method orig = class_getInstanceMethod(origClass, sel);
        IMP imp = imp_implementationWithBlock(^NSRect(id /*self*/, NSRect rect, NSScreen* /*scr*/) {
            return rect;
        });
        class_addMethod(newClass, sel, imp, method_getTypeEncoding(orig));
        objc_registerClassPair(newClass);
    }
    object_setClass(nsWindow, newClass);
}

// Promote a JUCE component's native NSWindow to a borderless screen-covering
// window above the macOS menu bar, and hide menu bar + dock at the app level.
// JUCE's setKioskModeComponent on macOS only resizes the window inside the
// visible (menu-bar-excluded) screen frame — it doesn't elevate the window
// level or set NSApp presentation options, so the menu bar stays visible.
//
// CGShieldingWindowLevel() is the highest userland window level (used by the
// system for things like the screen lock dialog), well above the menu bar.
void macEnterTrueFullscreen(juce::Component& comp)
{
    auto* peer = comp.getPeer();
    if (peer == nullptr) return;

    NSView* nsView = (__bridge NSView*) peer->getNativeHandle();
    NSWindow* nsWindow = [nsView window];
    if (nsWindow == nil) return;

    NSScreen* screen = [nsWindow screen] ?: [NSScreen mainScreen];
    NSRect screenFrame = [screen frame];

    dvUnconstrainWindow(nsWindow);

    [nsWindow setStyleMask: NSWindowStyleMaskBorderless];
    [nsWindow setBackgroundColor: [NSColor blackColor]];
    [nsWindow setOpaque: YES];
    [nsWindow setHasShadow: NO];
    [nsWindow setLevel: CGShieldingWindowLevel()];
    [nsWindow setFrame: screenFrame display: YES animate: NO];
    [nsWindow orderFrontRegardless];
    [nsWindow makeKeyAndOrderFront: nil];

    [NSApp setPresentationOptions:
        NSApplicationPresentationHideMenuBar
      | NSApplicationPresentationHideDock];
}

// Restore the window and app to a normal state. Order matters: presentation
// options first (so the menu bar/dock are back before window destruction
// races with the WindowServer; on macOS 26, doing this last left global
// input in a captured state and required a hard reset). Then drop the level,
// hide the window, and restore the original class so dealloc runs on a real
// Cocoa class instead of our dynamic subclass.
void macExitTrueFullscreen(juce::Component& comp)
{
    [NSApp setPresentationOptions: NSApplicationPresentationDefault];

    if (auto* peer = comp.getPeer())
    {
        NSView* nsView = (__bridge NSView*) peer->getNativeHandle();
        NSWindow* nsWindow = [nsView window];
        if (nsWindow != nil)
        {
            [nsWindow setLevel: NSNormalWindowLevel];
            [nsWindow orderOut: nil];

            Class current = [nsWindow class];
            const char* name = class_getName(current);
            if (name != nullptr && strncmp(name, "DV_Unconstrained_", 17) == 0)
            {
                Class parent = class_getSuperclass(current);
                if (parent != nil)
                    object_setClass(nsWindow, parent);
            }
        }
    }
}

#endif // __APPLE__
