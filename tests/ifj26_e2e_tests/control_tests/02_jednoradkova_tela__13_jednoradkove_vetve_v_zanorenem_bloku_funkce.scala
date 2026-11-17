import ifj26.*

def show(n: Int): Unit =
  if n > 0 then print("+") else print("-")

@main
def main(): Unit =
  show(1)
  show(-1)
  show(2)
