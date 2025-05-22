# Covenant  Project

### How to Run
- Open the project in **Unreal Engine 5.4.4**
- There is only one map, and it should open by default
- Click **Play** and use **WASD**, **Space**, **Ctrl**, and the **mouse** to move

---

## Project Structure

The project follows the standard Unreal Engine structure. As suggested in the task description, I used both **Blueprints** and **C++** to implement the logic.

**Blueprints are used for:**
- A simple **GameMode**
- A simple **Pawn**
- The **PlayerController**

**C++ is used for:**
- Defining basic **interfaces**
- Handling **spawning and balancing behavior** in `CovShapeSpawner`

---

## Notable Decisions

The task didn’t specify how many shapes should be spawned. Assuming standard gameplay conditions, the number is likely relatively small and in that case, spawning a separate actor for each shape would be fine (and more flexible).  
However, I decided to support **a large number of shapes** instead.

To achieve that, I used **Instanced Static Mesh Components**. Each component can render a large number of shapes (with the same mesh and material pair) in a **single draw call**.  
Thanks to this approach, I can spawn **100,000+ shapes** in the editor with a reasonable frame rate — and all of them are **balanced**.

Of course, this comes at the cost of flexibility and slighlty more complex logic. For example adding per instance logic becomes much more difficult when using instanced rendering.

**Video showcase**: [https://www.youtube.com/watch?v=luOUUFE_Ym4](https://www.youtube.com/watch?v=luOUUFE_Ym4)