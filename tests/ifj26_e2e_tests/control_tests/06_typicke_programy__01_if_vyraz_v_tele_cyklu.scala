import ifj26.*

@main
def main(): Unit =
  for i <- 1 to 5 do
    val t = if i / 2 * 2 == i then 1 else 0
    print(t)
