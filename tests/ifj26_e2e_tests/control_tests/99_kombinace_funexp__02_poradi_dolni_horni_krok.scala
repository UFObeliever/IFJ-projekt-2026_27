import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  for i <- tr("a", 1) to tr("b", 3) by tr("c", 1) do print(i)
