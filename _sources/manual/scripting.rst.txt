.. _Scripting:

Scripting
===========
This scripting guide shows how to implement and use a Script Component in the game engine, detailing the available functions in Lua.

Editor
------
How to implement a Script Component?
________

1. Create a new project **(in an empty folder)** and save the current scene
2. Create an Entity and add to it a Script Component
3. Open the script from where you saved it, and the following should appear

.. code-block:: Lua
    :linenos: 
  
    function on_ready()
        -- Add initialization code here 
    end

    function on_update(dt)
        -- Add update code here 
    end

    function on_exit()
        -- Add cleanup code here
    end

The variables created as public with the following nomenclature, will appear on the inspector inside the Script Component, **on Runtime mode**:

.. code-block:: Lua
    :linenos: 
   
    --[[export]] age = 10


.. admonition:: Note
    :class: note

    For now, if a variable is public, any change made from the inspector won't apply to it.

Lua
----

Logging functions
________

The engine provides functions to register messages on the console with diferent levels of severity 

1. log(message)
   
.. code-block:: Lua
    :linenos:

    log("This is an informative message")

1. log_warning(message)

.. code-block:: Lua
    :linenos:

    log("This is a warning message")

1. log_error(message)

.. code-block:: Lua
    :linenos: 

    log("This is an error message")

1. log_critical(message)

.. code-block:: Lua
    :linenos:

    log("This is a critical message")

Input functions
________

1. Input.is_key_pressed(key)
Verifies if a specific key is being pressed

.. code-block:: Lua
    :linenos:

    if Input.is_key_pressed(Input.keycode.A) then 
        log("The key A is being pressed")
    end

2. Input.is_mouse_button_pressed(button)
Verifies if a mouse button is being pressed

.. code-block:: Lua
    :linenos:

    If Input.is_mouse_button_pressed(Input.mousecode.LEFT)
    then 
        log("The left mouse button is being pressed")
    end

3. Input.get_mouse_position()
Gets the actual position of the mouse on screen 

.. code-block:: Lua
    :linenos: 

    local x, y = Input.get_mouse_position()
    log("Mouse position: " .. x .. ", " .. y)

Mathematics functions (GLM)
________

1. Vectors
    1.1. Vector2
    Represents a vector2D

    .. code-block:: Lua
        :linenos:

        local vec2 = Vector2.new(1.0, 2.0)
        log("Vector2: " .. vec2.x .. ", " .. vec2.y)

    1.2. Vector3
    Represents a vector3D

    .. code-block:: Lua
        :linenos:

        local vec3 = Vector3.new(1.0, 2.0, 3.0)
        log("Vector3: " .. vec3.x .. ", " .. vec3.y .. ", " .. vec3.z)

    1.3. Vector4
    Represents a vector4D

    .. code-block:: Lua
        :linenos:

        local vec4 = Vector4.new(1.0, 2.0, 3.0, 4.0)
        log("Vector4: " .. vec4.x .. ", " .. vec4.y .. ", " .. vec4.z .. ", " .. vec4.w)

2. Matrix
    2.1. Mat4

    .. code-block:: Lua
        :linenos:

        local mat = Mat4()
        mat = mat:translate(Vector3(1.0, 2.0, 3.0))
        log("Translated matrix")

3. Quaternions
    3.1. Quaternion

    .. code-block:: Lua
        :linenos:

        local quat = Quaternion.new(1.0, 0.0, 0.0, 0.0)
        log("Quaternion: " .. quat.x .. ", " .. quat.y .. ", " .. quat.z .. ", " .. quat.w)

Entity functions
________

Entities are objects in the scene that can have associated components

1. Creation and destruction of Entities
    1.1. current_scene.create_entity()
    Creates a new entity in the current scene

    .. code-block:: Lua
        :linenos:

        local entity = current_scene.create_entity()

    1.2. current_scene.destroy_entity(entity)
    Destroys an entity in the current scene

    .. code-block:: Lua
        :linenos:
        
        current_scene.destroy_entity(entity)

.. admonition:: Note
    :class: note

    If needed to refer to the entity running the script, instead of using "entity:", we use "self:" in the following functions. 
    For instance, instead of entity:get_component("ComponentName"), use self:get_component("ComponentName") to refer to the entity that contains the script. 

2. Components
    2.1. Entity.add_component(componentName)
    Adds a component to the entity

    .. code-block:: Lua
        :linenos:

        entity:add_component("TransformComponent")

    2.2. Entity.get_component(componentName)
    Gets a component of the entity 

    .. code-block:: Lua
        :linenos:

        local transform = entity:get_component("TransformComponent")

    2.3. Entity.has_component(componentName)
    Verifies if the entity has a specific component

    .. code-block:: Lua
        :linenos:

        if entity:has_component("TransformComponent")
        then
            log("The entity has a transformation component")
        end

    2.4. Entity.remove_component(componentName)
    Deletes a component from the entity

    .. code-block:: Lua
        :linenos:

        entity:remove_component("TransformComponent")

3. Entity hierarchy
    3.1. Entity.set_parent(parent)
    Sets the parent entity 

    .. code-block:: Lua
        :linenos:

        entity:set_parent(parentEntity)

    3.2. Entity.get_parent()
    Gets the parent entity

      .. code-block:: Lua
        :linenos:

        local parent = entity:get_parent()

    3.3. Entity.get_child()
    Gets the first child of the entity

      .. code-block:: Lua
        :linenos:

        local child = entity:get_child()

    3.4. Entity.get_children()
    Gets all entity children 

      .. code-block:: Lua
        :linenos:

        local children = entity:get_children()

Components
________

1. TagComponent
Stores a label for the entity 

.. code-block:: Lua
    :linenos:

    local tag = entity:get_component("TagComponent")
    tag.tag = "Player"

2. TransformComponent
Stores the position, rotation and scale of the entity

.. code-block:: Lua
    :linenos:

    local transform = entity:get_component("TransformComponent")
    transform.position = Vector3(1.0, 2.0, 3.0)

3. CameraComponent
Represents a camera 

.. code-block:: Lua
    :linenos:

    local camera = entity:get_component("CameraComponent")

4. MeshComponent
Represents a 3D mesh 

.. code-block:: Lua
    :linenos:

    local mesh = entity:get_component("MeshComponent")

5. MaterialComponent
Represents a material 

.. code-block:: Lua
    :linenos:

    local material = entity:get_component("MaterialComponent")

6. LightComponent
Represents a light 

.. code-block:: Lua
    :linenos:

    local light = entity:get_component("LightComponent")
    light.color = Vector3(1.0, 1.0, 1.0)

7. ScriptComponent
Allows scripts to be attached to the entity 

.. code-block:: Lua
    :linenos:

    local script = entity:get_component("ScriptComponent")
    script:call("update")

Scenes
________

1. current_scene.get_entity_by_name(name)
Gets an entity by name

.. code-block:: Lua
    :linenos:

    local entity = current_scene.get_entity_by_name("Player")

2. current_scene.get_all_entities()
Gets all entities in the current scene

.. code-block:: Lua
    :linenos:

    local entities = current_scene.get_all_entities()


Script Execution
________

1. LuaBackend.CreateScript(path)
Creates a script from a file 

.. code-block:: Lua
    :linenos:

    auto script = LuaBackend::CreateScript("path/to/script.lua");

2. LuaBackend.ExecuteScript(script)
Executes a script

.. code-block:: Lua
    :linenos:

    LuaBackend::ExecuteScript(*script);





