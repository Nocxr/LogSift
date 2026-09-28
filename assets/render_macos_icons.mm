#import <Cocoa/Cocoa.h>

static NSRect R(CGFloat x, CGFloat y, CGFloat w, CGFloat h, CGFloat s) {
    return NSMakeRect(x * s, y * s, w * s, h * s);
}

static void FillRound(NSRect rect, CGFloat radius, NSColor* color) {
    [color setFill];
    [[NSBezierPath bezierPathWithRoundedRect:rect xRadius:radius yRadius:radius] fill];
}

static void DrawAppIcon(NSBitmapImageRep* rep, CGFloat s) {
    [NSGraphicsContext saveGraphicsState];
    NSGraphicsContext* gc = [NSGraphicsContext graphicsContextWithBitmapImageRep:rep];
    [NSGraphicsContext setCurrentContext:gc];

    [[NSColor clearColor] setFill];
    NSRectFill(NSMakeRect(0, 0, s, s));

    const CGFloat pad = 0.055 * s;
    NSRect body = NSMakeRect(pad, pad, s - 2.0 * pad, s - 2.0 * pad);
    NSBezierPath* bg = [NSBezierPath bezierPathWithRoundedRect:body
                                                      xRadius:0.19 * s
                                                      yRadius:0.19 * s];
    NSGradient* gradient = [[NSGradient alloc]
        initWithStartingColor:[NSColor colorWithCalibratedRed:0.065 green:0.105 blue:0.145 alpha:1.0]
                  endingColor:[NSColor colorWithCalibratedRed:0.015 green:0.035 blue:0.050 alpha:1.0]];
    [gradient drawInBezierPath:bg angle:90.0];

    [[NSColor colorWithCalibratedWhite:0.10 alpha:0.75] setStroke];
    [bg setLineWidth:0.006 * s];
    [bg stroke];

    NSColor* gray = [NSColor colorWithCalibratedRed:0.48 green:0.56 blue:0.64 alpha:1.0];
    NSColor* grayDim = [NSColor colorWithCalibratedRed:0.28 green:0.34 blue:0.40 alpha:1.0];
    NSColor* red = [NSColor colorWithCalibratedRed:1.00 green:0.24 blue:0.14 alpha:1.0];
    NSColor* green = [NSColor colorWithCalibratedRed:0.05 green:0.92 blue:0.39 alpha:1.0];

    const CGFloat r = 0.012 * s;
    FillRound(R(0.15,0.76,0.055,0.020,s), r, grayDim);
    FillRound(R(0.22,0.76,0.225,0.020,s), r, gray);
    FillRound(R(0.15,0.69,0.115,0.020,s), r, gray);
    FillRound(R(0.29,0.69,0.155,0.020,s), r, red);
    FillRound(R(0.15,0.62,0.245,0.020,s), r, gray);
    FillRound(R(0.41,0.62,0.055,0.020,s), r, grayDim);
    FillRound(R(0.15,0.55,0.290,0.020,s), r, gray);
    FillRound(R(0.15,0.48,0.055,0.020,s), r, grayDim);
    FillRound(R(0.22,0.48,0.155,0.020,s), r, red);
    FillRound(R(0.39,0.48,0.090,0.020,s), r, gray);
    FillRound(R(0.15,0.41,0.315,0.020,s), r, gray);
    FillRound(R(0.15,0.34,0.045,0.020,s), r, grayDim);
    FillRound(R(0.21,0.34,0.095,0.020,s), r, red);
    FillRound(R(0.32,0.34,0.145,0.020,s), r, gray);
    FillRound(R(0.15,0.27,0.225,0.020,s), r, gray);
    FillRound(R(0.39,0.27,0.075,0.020,s), r, grayDim);
    FillRound(R(0.15,0.20,0.135,0.020,s), r, gray);
    FillRound(R(0.30,0.20,0.145,0.020,s), r, red);

    FillRound(R(0.56,0.76,0.295,0.020,s), r, gray);
    FillRound(R(0.56,0.69,0.070,0.020,s), r, grayDim);
    FillRound(R(0.65,0.69,0.205,0.020,s), r, gray);
    FillRound(R(0.56,0.62,0.280,0.020,s), r, gray);
    FillRound(R(0.57,0.53,0.250,0.020,s), r, gray);
    FillRound(R(0.69,0.46,0.170,0.024,s), r, green);
    FillRound(R(0.69,0.37,0.170,0.024,s), r, green);
    FillRound(R(0.60,0.27,0.215,0.020,s), r, gray);
    FillRound(R(0.56,0.20,0.285,0.020,s), r, gray);

    NSBezierPath* sieve = [NSBezierPath bezierPath];
    [sieve moveToPoint:NSMakePoint(0.46*s, 0.78*s)];
    [sieve curveToPoint:NSMakePoint(0.52*s, 0.52*s)
          controlPoint1:NSMakePoint(0.52*s, 0.78*s)
          controlPoint2:NSMakePoint(0.43*s, 0.58*s)];
    [sieve curveToPoint:NSMakePoint(0.47*s, 0.18*s)
          controlPoint1:NSMakePoint(0.66*s, 0.43*s)
          controlPoint2:NSMakePoint(0.45*s, 0.32*s)];
    [sieve setLineWidth:0.045 * s];
    [sieve setLineCapStyle:NSLineCapStyleRound];
    [sieve setLineJoinStyle:NSLineJoinStyleRound];
    [[NSColor colorWithCalibratedRed:0.67 green:0.75 blue:0.82 alpha:1.0] setStroke];
    [sieve stroke];

    [NSGraphicsContext restoreGraphicsState];
}

static void DrawTray(NSBitmapImageRep* rep, CGFloat s) {
    [NSGraphicsContext saveGraphicsState];
    [NSGraphicsContext setCurrentContext:[NSGraphicsContext graphicsContextWithBitmapImageRep:rep]];
    [[NSColor clearColor] setFill];
    NSRectFill(NSMakeRect(0, 0, s, s));

    NSColor* ink = [NSColor blackColor];
    const CGFloat rr = 0.035 * s;
    FillRound(R(0.10,0.72,0.26,0.070,s), rr, ink);
    FillRound(R(0.10,0.55,0.34,0.070,s), rr, ink);
    FillRound(R(0.10,0.38,0.27,0.070,s), rr, ink);
    FillRound(R(0.10,0.21,0.31,0.070,s), rr, ink);
    FillRound(R(0.64,0.67,0.25,0.070,s), rr, ink);
    FillRound(R(0.68,0.49,0.22,0.070,s), rr, ink);
    FillRound(R(0.64,0.29,0.25,0.070,s), rr, ink);

    NSBezierPath* sieve = [NSBezierPath bezierPath];
    [sieve moveToPoint:NSMakePoint(0.44*s,0.82*s)];
    [sieve curveToPoint:NSMakePoint(0.57*s,0.51*s)
          controlPoint1:NSMakePoint(0.57*s,0.80*s)
          controlPoint2:NSMakePoint(0.43*s,0.61*s)];
    [sieve curveToPoint:NSMakePoint(0.46*s,0.16*s)
          controlPoint1:NSMakePoint(0.71*s,0.43*s)
          controlPoint2:NSMakePoint(0.42*s,0.31*s)];
    [sieve setLineWidth:0.10*s];
    [sieve setLineCapStyle:NSLineCapStyleRound];
    [sieve setLineJoinStyle:NSLineJoinStyleRound];
    [ink setStroke];
    [sieve stroke];

    [NSGraphicsContext restoreGraphicsState];
}

static bool WritePNG(NSString* path, NSInteger size, bool tray) {
    NSBitmapImageRep* rep = [[NSBitmapImageRep alloc]
        initWithBitmapDataPlanes:NULL
                      pixelsWide:size
                      pixelsHigh:size
                   bitsPerSample:8
                 samplesPerPixel:4
                        hasAlpha:YES
                        isPlanar:NO
                  colorSpaceName:NSCalibratedRGBColorSpace
                     bytesPerRow:0
                    bitsPerPixel:0];
    if (!rep) return false;
    if (tray) DrawTray(rep, (CGFloat)size);
    else DrawAppIcon(rep, (CGFloat)size);
    NSData* png = [rep representationUsingType:NSBitmapImageFileTypePNG properties:@{}];
    return [png writeToFile:path atomically:YES];
}

int main(int argc, const char* argv[]) {
    @autoreleasepool {
        if (argc != 2) return 2;
        NSString* outDir = [NSString stringWithUTF8String:argv[1]];
        NSString* iconset = [outDir stringByAppendingPathComponent:@"LogSift.iconset"];
        NSFileManager* fm = [NSFileManager defaultManager];
        [fm createDirectoryAtPath:iconset withIntermediateDirectories:YES attributes:nil error:nil];

        struct Entry { const char* name; int size; };
        const Entry entries[] = {
            {"icon_16x16.png",16}, {"icon_16x16@2x.png",32},
            {"icon_32x32.png",32}, {"icon_32x32@2x.png",64},
            {"icon_128x128.png",128}, {"icon_128x128@2x.png",256},
            {"icon_256x256.png",256}, {"icon_256x256@2x.png",512},
            {"icon_512x512.png",512}, {"icon_512x512@2x.png",1024}
        };
        for (const Entry& e : entries) {
            NSString* path = [iconset stringByAppendingPathComponent:[NSString stringWithUTF8String:e.name]];
            if (!WritePNG(path, e.size, false)) return 3;
        }

        NSString* tray = [outDir stringByAppendingPathComponent:@"LogSiftTray.png"];
        if (!WritePNG(tray, 44, true)) return 4;
    }
    return 0;
}
