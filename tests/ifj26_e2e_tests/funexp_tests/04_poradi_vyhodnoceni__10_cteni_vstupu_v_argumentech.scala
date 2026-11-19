import ifj26.*

def sub(a: Int, b: Int): Int = a - b

@main
def main(): Unit =
  print(toStr(sub(readInt(), readInt())))
