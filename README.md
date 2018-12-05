# cppreactor
Single threaded coroutines run in a single scheduler (reactor). Useful to implement actor-like approaches with actions that take many frames.

<Add example code>

This project is experimental and is used to test out the capabilities of the new Coroutine TS that will hopefully be available in C++ 20. 

The project is natural decendant of Coroutines project for C# with clear goals:
- [personal goal] use newly acquired C++ knowledge of Coroutine TS and template metaprogramming to try out new design
- use C++'s Coroutines to full extend to increase performance and usability compared to C# version (C# version is extension of Unity's Coroutines)
- use policy based design where needed to allow user to customize behaviour
- try to improve or at least match the capabilities of C# version - supply date time
- clearly compare C++ and C# version to estimate the performance gain/loss
