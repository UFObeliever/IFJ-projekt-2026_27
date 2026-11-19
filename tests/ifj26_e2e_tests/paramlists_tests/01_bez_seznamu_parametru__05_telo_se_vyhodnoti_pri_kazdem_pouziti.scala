import ifj26.*

def tick: Int =
  print("t")
  1

@main
def main(): Unit =
  val a = tick + tick + tick
  print(a)
