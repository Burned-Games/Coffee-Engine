.. _Visual Studio:

Visual Studio
=============

`Visual Studio Community <https://visualstudio.microsoft.com>`__ is a Windows-only IDE
by `Microsoft <https://microsoft.com>`_ that's free for individual use or non-commercial use within organizations.
It has many useful features, such as memory view, performance view, source
control and more.

---------------------------------------

1. Enable vcpkg
---------------

If you have administrator permissions:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- `vcpkg is now included with Visual Studio <https://devblogs.microsoft.com/cppblog/vcpkg-is-now-included-with-visual-studio/>`_


If you don't have administrator permissions:
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Use the script (recommended option):**
  
  1. Download the script from `here <https://gist.github.com/hugoplanell/e6b77b8d4ea8f6daa9b4d32e10a34bca/archive/9a8fcd49a04f29380c705e42c8df433588c96c2c.zip>`_
  2. Navigate to the directory where the script was downloaded.
  3. Run the script with the following command:

     .. code-block:: console

        .\vcpkg-without-admin-rights.bat

**Manual installation:**

  1. Go to the user folder (optional):

     .. code-block:: console

        cd %UserProfile%

  2. Clone vcpkg in a folder of your choice:

     .. code-block:: console

        git clone https://github.com/microsoft/vcpkg.git

  3. Bootstrap vcpkg:

     .. code-block:: console

        cd vcpkg
        .\bootstrap-vcpkg.bat

  4. Integrate this vcpkg instance into the system:

     .. code-block:: console

        .\vcpkg.exe integrate install

  5. Set the user environment variable:

     .. code-block:: console

        setx VCPKG_ROOT .

---------------------------------------

2. Open the project
--------------------

- Two ways:
  - Right-click on the project folder and select **Open with Visual Studio**.
  - Open Visual Studio 2022 and use the **Open a Local Folder** option.

3. Once Visual Studio is opened
--------------------------------

1. Ensure there are no errors and that **-- Running vcpkg install** appears in the output window.

.. image:: img/Ventana-Salida-Correcto-Primera-Apertura.png
   :alt: Output window showing vcpkg install running

1. Wait until you see **CMake generation has finished** in the output.

.. image:: img/Ventana-Salida-Correcto-Esperar.png
   :alt: Output window showing CMake generation finished

Run the Engine
--------------

1. Select the project to compile:

.. image:: img/Seleccionar-Proyecto.png
   :alt: Selecting project to compile

2. Press the **Play** button:

.. image:: img/Ejecutar.png
   :alt: Press the Play button to run the project

3. Hope nothing fails 0-o
-------------------------

1. Enjoy!
---------

.. image:: img/Resultado.png
   :alt: The result after running the engine
