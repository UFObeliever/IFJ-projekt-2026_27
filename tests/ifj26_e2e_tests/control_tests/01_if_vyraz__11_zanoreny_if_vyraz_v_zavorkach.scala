import ifj26.*

@main
def main(): Unit =
  val a = true
  val b = false
  val r = if a then (if b then 1 else 2) else 3
  print(r)
