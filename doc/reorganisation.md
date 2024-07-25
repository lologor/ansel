# Ansel code base reorganization

## The current problem

Ansel is not modular, as the [/src dependency graph](@ref src) shows: everything is wired to the GUI, "modules" are all aware of everything. Modifying anything __somewhere__ typically breaks something unexpected __somewhere else__.

## Target design

_Note: because of the plotting library, this is a reversed dependency graph_.

<pre class="mermaid">
flowchart TD
  libs(libs) --> math(math)
  libs --> string(string)
  libs --> CPU(CPU)
  libs --> path(path)
  libs --> color(colorspaces)
  app --> database
  app --> GUI
  app --> iop
  GUI --> views
  views --> darkroom
  views --> lighttable
  history --> node[pipeline nodes]
  history --> database
  history --> styles
  node --> pipe[pixel pipeline]
  iop[IOP modules] --> node
  iop --> history
  database --> lighttable
  darkroom --> pipe
  darkroom --> iop
</pre>

We should here make distinctions between:

- __libs__ : basic shared libraries, within the scope of the project, that do basic-enough reusable operation, unaware of app-wise datatypes and data (plotted with rounded corners).
- __modules__ (in the programming sense) : self-enclosed code units that cover high-level functionnality (plotted with square corners).


### IOP modules

IOP (_Image OPerations_) modules are more like plugins: it's actually how they are referred to in many early files. They define both a _pipeline node_ (aka pixel filtering code) and a GUI widget in darkroom. The early design shows initial intent of making them re-orderable in the pipelpine, and to allow third-party plugins. As such, the core was designed to be unaware of IOP internals.

As that initial project seemed to be abandonned, IOP modules became less and less enclosed from the core, which allowed some lazy mixes and confusions between what belongs to the scope of the pipeline, and what belongs to the scope of modules. The pipeline output profile can therefore be retrieved from the _colorout_ module or from the pipeline data. _color calibration_ reads the input profile from _colorin_.

IOP modules also commit their parameter history directly using [`darktable.develop`](@ref darktable_t) global data, instead of using their private link [`(dt_iop_module_t *)->dev`](@ref dt_iop_module_t), which is documented in an old comment to be the only thread-safe way of doing it.

TODO:

- [ ] make IOP modules only aware of history and GUI object,
- [ ] handle uniform ways of communicating between modules on the pipeline

### History

The editing history is the snapshot of parameters for each IOP. It gets saved to the database. It gets read and flattened to copy parameters to pipeline nodes. The problem of the history code, right now, is it was hacked with masks later, so it mixes SQL code, GUI code, pixelpipe code and… well, history code.

History should only deal with push/pop/merge operations and communicate with pipeline through a flattened associative list: 1 parameter state <-> 1 pipeline node.


### Develop

The [development](@ref /src/develop/develop.h) is an hybrid thing joining history with pipeline. This object will take care of reading the image cache to grab the buffer, reading the database history, initing a pipeline, and starting a new computing job.

But again, it has been hacked to lazily handle in a centralized fashion many things that only belong to the darkroom, like the colorpicker proxies, overexposed/gamut alerts, ISO 12626 preview, and such. Those belong to GUI, because development objects are also used when exporting images or rendering thumbnails.
