# cppreactor
Single threaded coroutines run in a single scheduler (reactor). Useful to implement actor-like approaches with actions that take many frames.

```
reactor_coroutine_return<card> get_card(card_deck& deck)
{
   while(true)
   {
      // Try to get card from deck, if any available (poll)
      card c;
      if(deck.try_get(c))
         co_return c;

      // Wait one scheduler "frame"
      co_await next_frame{};
   }
}

reactor_coroutine<> deal_cards(card_deck& deck)
{
   auto card1 = co_await get_card(deck);
   // .. Do sth with card

   auto card2 = co_await get_card(deck);

   // ...
}

reactor_scheduler<> scheduler;
scheduler.push(do_cards(deck));

// ...

// In a loop
scheduler.update_next_frame();

```

## Goal

This project is experimental and is used to test out the capabilities of the new Coroutine TS that will hopefully be available in C++ 20. 

The project is natural decendant of [Coroutines project for C#](https://github.com/zigaosolin/Coroutines) with clear goals:
- [personal goal] use newly acquired C++ knowledge of Coroutine TS and template metaprogramming to try out new design
- use C++'s Coroutines to full extend to increase performance and usability compared to C# version (C# version is extension of Unity's Coroutines)
- try to improve or at least match the capabilities of C# version - supply date time
- clearly compare C++ and C# version to estimate the performance gain/loss

## Features

* Supports coroutines with and without return values
```
reactor_coroutine<> void_coroutine() { ... }
reactor_coroutine_return<float> float_coroutine { ... }
```

* The lowest level suspend is wait for next frame
```
auto frame_data = co_await next_frame{};
```

* Frame data can be customized to anything you want but all coroutines in the same scheduler are forced to use the same:
```
reactor_coroutine<frame_struct> 
reactor_coroutine_return<float, frame_struct> 
auto frame_data = co_await next_frame<frame_struct>{};
```

* You are in total control of schedulers, and can pause/resume them at any time you want. You can also move them to different threads between frames. All you need to call is
```
reactor_scheduler<> scheduler;

// .. may push reactor_coroutine<> here

scheduler.next_frame_update(frame_data);
```

## Performance

I am very pleased with the performance. For a simple infinite loop test
```
reactor_coroutine<> performance_test()
{
   for(;;)
     co_await next_frame{};
}
```

I am able to perform around *70M* frame updates per second. Compared to *11M* for similar setup in C#, I think this is really good.



