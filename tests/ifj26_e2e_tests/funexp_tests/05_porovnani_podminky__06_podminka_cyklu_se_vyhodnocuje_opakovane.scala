import ifj26.*

def tr(t: String, v: Int): Int =
  print(t)
  v

@main
def main(): Unit =
  var i = 0
  while tr("c", i) < 3 do
    i = i + 1
