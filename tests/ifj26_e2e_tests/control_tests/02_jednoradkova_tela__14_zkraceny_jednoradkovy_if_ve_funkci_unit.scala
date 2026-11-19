import ifj26.*

def maybe(n: Int): Unit =
  if n > 0 then print("p")

@main
def main(): Unit =
  maybe(1)
  maybe(-1)
  maybe(2)
