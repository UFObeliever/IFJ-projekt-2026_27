import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  for i <- 1 to tr("n", 3) do print(i)
