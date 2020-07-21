###########
Development
###########

Useful tips for maintaining and adding to the project.


Enable git hooks
================

From within the repository, run

    ``git config --local core.hooksPath .githooks``



Installing and maintaining third-party libraries
================================================

catch
-----

**Add new submodule**

.. code-block::

    git submodule add -- https://github.com/catchorg/Catch2.git third_party/catch2/2.12.2
    cd third_party/catch2/2.12.2
    git checkout v2.12.2


**Remove old submodule**

.. code-block::

    vim .gitmodules
    vim .git/config
    git add .gitmodules
    git rm --cached third_party/catch2/2.12.1
    rm -rf .git/modules/third_party/catch2/2.12.1
    rm -rf third_party/catch2/2.12.1

**Point makefile to new version**

.. code-block::

    vim nrmake/third_party.mk


fmt
---

**Add new submodule**

.. code-block::

    git submodule add -- https://github.com/fmtlib/fmt.git third_party/fmt/7.0.0
    cd third_party/fmt/7.0.0
    git checkout 7.0.0

**Remove old submodule**

.. code-block::

    vim .gitmodules
    vim .git/config
    git add .gitmodules
    git rm --cached third_party/fmt/6.2.1
    rm -rf .git/modules/third_party/fmt/6.2.1
    rm -rf third_party/fmt/6.2.1

**Point makefile to new version**

.. code-block::

    vim nrmake/third_party.mk
