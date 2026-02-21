//always lit logo
struct LogoLight : SvgWidget {
    MODULE_NAME* module;
    LogoLight() {
        this->setSvg(Svg::load(asset::plugin(pluginInstance, "res/Logo.svg")));

    }

    void drawLayer(const DrawArgs& args, int layer) override {

        nvgGlobalCompositeBlendFunc(args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
        if (!svg) return;
        if (this->module && layer == 1) {
            for (auto s = svg->handle->shapes; s; s = s->next) {
                nvgStrokeWidth(args.vg, (s->strokeWidth));
                nvgFillColor(args.vg, nvgHSL(0.3f, 0.8f, 0.6f));
                for (auto p = s->paths; p; p = p->next) {
                    nvgBeginPath(args.vg);
                    nvgMoveTo(args.vg, p->pts[0], p->pts[1]);
                    for (auto i = 0; i < p->npts - 1; i += 3) {
                        float* path = &p->pts[i * 2];
                        nvgBezierTo(args.vg, path[2], path[3], path[4], path[5], path[6], path[7]);
                    }
                    if (p->closed)
                        nvgLineTo(args.vg, p->pts[0], p->pts[1]);
                    if (s->fill.type)
                        nvgFill(args.vg);
                    if (s->stroke.type)
                        nvgStroke(args.vg);
                }
            }
        }
        Widget::drawLayer(args, layer);
    }
};