#pragma once
#include <QImage>
#include <QSize>
namespace multra {
enum class CanvasImageScope { network,view };
struct CanvasImageOptions {
    CanvasImageScope scope{CanvasImageScope::network};
    QSize pixels{2048,1536};
    bool background{true};
};
// Authoring-only raster: live display/level/wireframe, no selection, drafts, grid or Run.
// Max 8192 per edge and 32 million pixels; throws EDIT_IMAGE_SIZE before allocation.
void validateCanvasImageSize(QSize);
}
