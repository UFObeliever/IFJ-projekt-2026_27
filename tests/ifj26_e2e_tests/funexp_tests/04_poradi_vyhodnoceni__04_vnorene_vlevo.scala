import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def add(a: Int, b: Int): Int = a + b

@main
def main(): Unit =
  print("|" + toStr(add(add(tr("a", 1), tr("b", 2)), tr("c", 3))))
