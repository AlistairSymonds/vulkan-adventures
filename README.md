# vulkan-adventures
WARNING. BAD CODE INSIDE.

In the words of Ralph Wiggum, I'm learneding! 


Notes:
VkOhNo contains:
- Camera
- Vector of renderables
	- Material ids
	- Texture ids
	- Mesh ids
	- World matrix
- Loaded textures by id
- Loaded Meshes by id

Render Engine contains:
- Per engine handling of materials (per primitive pipelines)
- Per render engine pipelines
- Tweakable heierachy for imgui stuff
- eg a raster vs rt engine

So to render you would pass an array of Renderables and the stored data with ids
Then render engine interprets those by material model and goes brrr
