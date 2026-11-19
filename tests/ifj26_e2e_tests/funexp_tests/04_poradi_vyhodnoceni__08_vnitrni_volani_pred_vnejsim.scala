import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

def outer(x: Int): Int =
  print("o")
  x

@main
def main(): Unit =
  val r = outer(tr("i", 1)) + tr("j", 2)
  print("|" + toStr(r))
