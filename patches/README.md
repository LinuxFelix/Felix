# Felix patches

`make sources` obtains these upstream revisions and applies the matching patch:

| Component | Upstream | Revision |
| --- | --- | --- |
| TinyX | https://github.com/tinycorelinux/tinyx | feab72ca891bc04b18763763e15ee4e532369cdf |
| flwm | https://github.com/tinycorelinux/flwm | 196f61c036ef65d65d5ec4095667f9a2d21822e2 |
| wbar | https://github.com/SpartanJ/wbar | f29900c82bf9184a29ba7c608a95cd0b714df8ec |

The patches retain Felix's build compatibility, framebuffer fixes, menu memory
fixes, glass-style decorations and dock behavior. Upstream source trees are
ignored by Git. Patches remain subject to the corresponding upstream licenses;
downloaded sources include those notices. Felix-specific source files carry
their own notices where present.

After changing a prepared vendor tree, maintainers can run
`python3 tools/export-vendor-patches.py` to refresh patches. Review the diff
before committing; the helper excludes generated TinyX build support files.
