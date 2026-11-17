import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  for i <- 1 to 3 if tr("g", 1) == 1 do print(i)
