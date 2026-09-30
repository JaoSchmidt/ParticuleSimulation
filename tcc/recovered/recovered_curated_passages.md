# Curated recoverable passages from Neovim undo history

These are **historical drafts**, not a verified snapshot of yesterday’s final document. Selected passages have complete sentences or coherent standalone content. Near-duplicate revisions, unfinished words, broken sentences, typing experiments and most text already in Git were excluded. Original wording is preserved, including grammatical mistakes. References such as `[9]` and `[11]` are from the undo history and may not match the Git version’s bibliography.

## Abstract and introduction

### Recovered passage — undo byte offset 21712

This work explores the integration of Entity Component System (ECS) and scripting into game engine development, with a focus on their roles in enhancing performance, maintainability, modularity and coding complexity. ECS provides a data-driven approach to managing game entities and behaviors, addressing challenges related to memory locality, polymorphism overhead, and parallelism. Scripting facilitates both dynamic and static interaction with the engine, through Lua and C++ respectivelly, allowing rapid iteration and greater accessibility when the ECS is unnecessary. This study analyzes their impact on core functionalities like graphics rendering and physics simulation. Key contributions include performance benchmarks, architectural insights, and practical guidelines for integrating ECS and scripting into game engines, with implications extending to simulation software and other performance-critical applications. This research aims to establish a robust foundation for future developments in modular and efficient game engine design.

### Recovered passage — undo byte offset 54551

The creation of game engines is a complex task that involves integrating multiple systems, including graphics, physics, input handling, sound, events, camera, just to name a few. One of the main architectural design choice of most modern game engines is the presence of ECS, which forbades object methods and, instead, strips its data into engine components. Alongside ECS, scripting plays a pivotal role in enabling dynamic interaction with the engine with interpreted languages, such as Python, Lua, among others. Both ECS and scripting allow for modular, maintainable, and scalable engine design, which is essential for handling the complexity of modern games.

### Recovered passage — undo byte offset 79269

This TCC focuses on how these systems are built and their relationship with other engine components, mainly graphics and physics. The goal is to design and implement these systems within a game engine, providing a deep comparasion with OOP of their impact on performance and then maintainability. Additionally, this work will explore how ECS and scripting influence other core systems, such as graphics rendering and physics simulations, demonstrating their importance in the overall architecture of game engines.

### Recovered passage — undo byte offset 116560

Scripting, meanwhile, allows game developers to still use OOP when necessary and, allow to quickly change gameplay without touching the engine code; which in turn, also makes it easier to apply modifications by non-core developers. Understanding how scripting interact with the ECS Api can provide valuable insights for improving engine performance and decide when to pass script code into the engine code.

## References and theoretical background

### Recovered passage — undo byte offset 198671

Computer Systems: A Programmer’s Perspective by Randal Bryant is sometimes called "The Computer Science Book" by some of my colleagues. This reference is key for understanding the lower-level assembly aspects of coding, which are critical when building high-performance and clean software. It covers topics like memory management, CPU architecture, instructions, unexpected behaviors and efficient code execution, all of which are relevant to optimizing queries inside ECS's systems, and their data structures.<cite>\[[1]\][1]</cite>

### Recovered passage — undo byte offset 215897

The EnTT source code by skypjack. EnTT is a well-established and highly efficient C++ library for implementing an Entity Component System (ECS). I chose to reference EnTT because it provides a already exhaustively tested ECS architecture that has been widely adopted in both hobbyist and professional game developments, like Minecraft, Diablo II, Call of Duty Vanguard, etc<cite>\[[2]\][2]</cite>. I also choose because of his wonderful tutorial called "ECS back and forth" which goes in detail on what problems engines are usually trying to solve. Studying EnTT's source code will expose some intersting ways on how to create functions to handle entities and components.

### Recovered passage — undo byte offset 235340

[3] S. Mertens, Flecs: A Fast Entity Component System (ECS) for C & C++. GitHub. [Online]. Available: https://github.com/sandermertens/flecs

### Recovered passage — undo byte offset 236605

[4] S. Mertens, “Building an ECS #1: Where are my Entities and Components,” Medium, Aug. 6, 2022. [Online]. Available: https://ajmmertens.medium.com/building-an-ecs-1-where-are-my-entities-and-components-63d07c7da742

### Recovered passage — undo byte offset 272079

Lastly and perhaps the closest to the present article is Sander Mertens Flecs ECS library, and also exhaustively tested on numerous projects like Tempest Rising, Territory Control 2, Resistance is Brutal, etc <cite>\[[9]\][9]</cite>. Which is also complemented by the amazing blog series "Building and ECS". There, he disclosures what designs and principles were choosen during his making of Flecs. Especially the main "Archetype" architecture, which differs from EnTT Sparse-Set.

## ECS motivation and comparison with Godot

### Recovered passage — undo byte offset 280581

There is no way around this subject other than to describe exactly what is being built here. As with any work, its purpose is to solve problems, or at least to solve it in a marginally better way than other existing solutions. To be more precise, there are 2 problems an ECS intends to solve for game engines:

### Recovered passage — undo byte offset 284049

Or, if you are found of inheritance and your language support multiple inheritance, you could also use it:

### Recovered passage — undo byte offset 299899

Even if your language doesn't support,it most likely support multiple interfaces. Meaning it can at least allow for polymorphism.

### Recovered passage — undo byte offset 306557

Either way, using those primary objects, you would then need to make it available for the developer, so it can be used on actual objects:

### Recovered passage — undo byte offset 310019

Therefore, you would need to define $2^N$ different classes, which is not feasable for large projects.

### Recovered passage — undo byte offset 323341

Therefore, you would need to define $2^N$ different classes, which is not feasable for higher values of $N$. Let's explore how Godot solve this problem inside its definiton of `Node` at `scene/main/node.h`. <cite>\[[11]\][11]</cite>

### Recovered passage — undo byte offset 420735

Now, the problem is solved by using composition, without using multiple inheritance. From Godot perspective, you can place new component inside its children hashmap, and as long as it is inherited from Node, then its ok.

### Recovered passage — undo byte offset 428539

At the end of the day, Godot solution makes the components ending up in a node tree.

### Recovered passage — undo byte offset 438640

Those direct approaches are prone to create two specific code styles, which I like to call "Vertical" and "Horizontal":

## Additional coherent historical references

- Offset 128066: [1] J. Linietsky, “Why isn’t Godot an ECS-based game engine?,” Godot Engine, Feb. 26, 2021. [Online]. Available: https://godotengine.org/article/why-isnt-godot-ecs-based-game-engine/

- Offset 320561: [5] Godot Engine Contributors, “Node class definition (node.h),” Godot Engine, GitHub. [Online]. Available: https://github.com/godotengine/godot/blob/master/scene/main/node.h

- Offset 325407: [11] Godot Engine Contributors, “Node class definition (node.h),” Godot Engine, GitHub. [Online]. Available: https://github.com/godotengine/godot/blob/master/scene/main/node.h

## Additional historical code and hierarchy examples

The following are preserved as historical examples, not validated executable code.

- Offset 330882: `GDCLASS(Node, Object);`

- Offset 331881: `void add_child(Node *child);`

- Offset 331917: `void remove_child(Node *child);`

- Offset 331956: `Node *get_parent() const;`

- Offset 331989: `Node *get_child(int index) const;`

- Offset 417421: `enemy.data.children.insert({"Mesh", new Mesh{}}, {"Audio", new Audio});`

## Exclusions

Discarded repeated keystroke-by-keystroke revisions, unfinished lines ending mid-sentence, malformed citation edits, case-only code edits, and text substantially duplicating the Git file. Some passages above are grammatically awkward but remain intact to avoid inventing words.
