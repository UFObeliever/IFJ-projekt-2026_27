import ifj26.*

def tick: Int =
  print("t")
  1

@main def main(): Unit =
  val x = tick + tick + tick
  print("\n")
  print(x)
  print("\n")
