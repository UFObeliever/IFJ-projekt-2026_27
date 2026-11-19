import ifj26.*

def tickA: Int =
  print("a")
  1

def tickB: Int =
  print("b")
  2

@main
def main(): Unit =
  val r = tickA * 10 + tickB
  print("|" + toStr(r))
