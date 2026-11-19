import ifj26.*

def maybe(x: Int): Unit =
  if x > 0 then
    print("p")

@main
def main(): Unit =
  maybe(1)
  maybe(-1)
  maybe(2)
