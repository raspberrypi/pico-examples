# Sample configuration files for VSCode

This example provides sample debug configurations that my be useful for developers using  the Microsoft(R) Visual Studio Code integrated development environment.

A basic setup for Visual Studio Code is described in [Getting Started with the Raspberry Pi Pico](https://rptl.io/pico-get-started). More advanced configurations may need additional plug-ins such as *cortex-debug*.

To use the files copy them into a `.vscode` folder at the top level of your project and adapt them to your needs (see the [Visual Studio documentation](https://code.visualstudio.com/docs/cpp/launch-json-reference)).

The files with names like `launch-*.json` illustrate different debugging configurations. You should select one of these, rename it to `launch.json` and make any desired modifications. Note that VS Code will ignore the others.

The `settings.json` file illustrates how to configure the VS Code *CMake* plug-in to ensure that you debug using *cortex-debug* instead of trying to launch the execuable on the host.