import ifj26.*

@main
def main(): Unit =
  val n = 10
  val t = if n > 5 then n.toString else "small"
  print(t)
