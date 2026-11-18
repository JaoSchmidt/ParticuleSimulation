-- Custom pandoc reader: reads trab_final_metpesq.md as it is written today and
-- reshapes it for the abnTeX2 template. The .md itself is never modified.

local CITACOES = {
  ["1"] = "bryant2011", ["2"] = "caini2024entt", ["3"] = "romeo2024ecst",
  ["4"] = "gregory2018", ["5"] = "unityentities", ["6"] = "driesen1996", ["7"] = "linietsky2021", ["8"] = "caini2019",
  ["9"] = "mertensflecs", ["10"] = "mertens2022", ["11"] = "godotnode",
  ["12"] = "entitysystemswiki", ["13"] = "wikipediaecs", ["14"] = "romeo2016",
  ["15"] = "bryant2011cap6",
}

-- Images drawn smaller than the default width (fraction of that width).
local ESCALAS = { ["images/matrix2.png"] = 0.5 }

local PADRAO_CITE = "<cite>\\%[%[(%d+)%]\\%]%[%d+%]</cite>?"

local function chave(n)
  return CITACOES[n] or error("citation [" .. n .. "] has no entry in CITACOES")
end

-- <cite>\[[7]\][7]</cite> => [@key]. ABNT puts the citation before the final period.
local function converter_citacoes(texto)
  texto = texto:gsub("%.%s*" .. PADRAO_CITE, function(n)
    return " [@" .. chave(n) .. "]."
  end)
  texto = texto:gsub("%s*" .. PADRAO_CITE, function(n)
    return " [@" .. chave(n) .. "]"
  end)
  -- Adjacent citations => one ABNT group: [@a] [@b] => [@a; @b]
  repeat
    local n
    texto, n = texto:gsub("(%[@[%w_]+)%]%s*%[@", "%1; @")
  until n == 0
  return texto
end

local function latex(inlines)
  local saida = pandoc.write(pandoc.Pandoc({ pandoc.Plain(inlines) }), "latex")
  return (saida:gsub("%s+$", ""))
end

-- For an italic "Figure 3: text" paragraph, returns the inlines after the label.
local function legenda(bloco, rotulo)
  if not bloco or bloco.t ~= "Para" or #bloco.content ~= 1 then return nil end
  local enfase = bloco.content[1]
  if enfase.t ~= "Emph" then return nil end
  local c = enfase.content
  if #c < 5 or c[1].t ~= "Str" or c[1].text ~= rotulo then return nil end
  if not (c[3].t == "Str" and c[3].text:match("^%d+:$")) then return nil end
  local resto = pandoc.Inlines({})
  for i = 5, #c do resto:insert(c[i]) end
  return resto
end

-- An image with its caption on the very next line (no blank line between them)
-- reaches us as one paragraph: Image, SoftBreak, Emph. Returns the caption's inlines.
local function legenda_colada(bloco)
  local c = bloco.content
  if #c ~= 3 or c[1].t ~= "Image" or c[2].t ~= "SoftBreak" then return nil end
  local paragrafo = pandoc.Para({ c[3] })
  return legenda(paragrafo, "Figure") or legenda(paragrafo, "Matrix")
end

local function fonte_de(caminho, meta)
  for prefixo, fonte in pairs(meta.fontes or {}) do
    if caminho:sub(1, #prefixo) == prefixo then
      return pandoc.utils.stringify(fonte)
    end
  end
  return nil
end

-- Caption below the figure (author's choice), then the source.
-- Figure labels for @fig:... references: {#fig:name} on an image or a code block.
local ROTULOS = {}
local function rotulo(id)
  if not id or id == "" then return "" end
  if not id:match("^fig:") then error("figure label must start with fig: (got #" .. id .. ")") end
  if ROTULOS[id] then error("figure label #" .. id .. " is used twice") end
  ROTULOS[id] = true
  return "\\label{" .. id .. "}"
end

local function figura(fig, inlines, meta)
  local imagem
  pandoc.walk_block(fig, { Image = function(i) imagem = imagem or i end })
  local fonte = fonte_de(imagem.src, meta)
  local largura = 0.85 * (ESCALAS[imagem.src] or 1)
  return pandoc.RawBlock("latex", table.concat({
    "\\begin{figure}[htbp]",
    "\\centering",
    "\\includegraphics[width=" .. largura .. "\\textwidth,height=0.6\\textheight,keepaspectratio]{" .. imagem.src .. "}",
    "\\caption{" .. latex(inlines) .. "}" .. rotulo(fig.identifier ~= "" and fig.identifier or imagem.identifier),
    fonte and ("\\fonte{" .. fonte .. "}") or "",
    "\\end{figure}",
  }, "\n"))
end

-- Code in listings because plain verbatim cannot wrap the long pseudocode lines.
-- With a "_Figure N: ..._" caption after it, the code becomes a figure: caption
-- below, kept in place (not floating) and listed in the List of Figures.
local function codigo(bloco, inlines)
  local opcoes = bloco.classes:includes("cpp") and "[language=C++]" or ""
  local lst = "\\begin{lstlisting}" .. opcoes .. "\n" .. bloco.text .. "\n\\end{lstlisting}"
  if not inlines then return pandoc.RawBlock("latex", lst) end
  return pandoc.RawBlock("latex", table.concat({
    "\\par\\medskip\\noindent\\begin{minipage}{\\linewidth}",
    lst,
    "\\figcaption{" .. latex(inlines) .. "}" .. rotulo(bloco.identifier),
    "\\end{minipage}\\par\\medskip",
  }, "\n"))
end

local function tabela(tab, inlines)
  if inlines then
    tab.caption.long = pandoc.Blocks({ pandoc.Plain(inlines) })
  end
  if #tab.colspecs > 6 then
    -- The row labels need more room than the source's dashes give them;
    -- take it from the narrow "cdots" columns (same width as column 3).
    local estreita = tab.colspecs[3][2]
    for _, spec in ipairs(tab.colspecs) do
      if spec[2] == estreita then spec[2] = estreita - 0.012 end
    end
    tab.colspecs[1][2] = tab.colspecs[1][2] + 0.036
    return {
      pandoc.RawBlock("latex", "\\begin{landscape}\\begingroup\\scriptsize\\setlength{\\tabcolsep}{3pt}"),
      tab,
      pandoc.RawBlock("latex", "\\endgroup\\end{landscape}"),
    }
  end
  return { tab }
end

-- Number comes from LaTeX, level comes from the number ("3.1" => section).
local function titulo(h, vistos, meta, saida)
  local primeiro = h.content[1]
  local num = primeiro and primeiro.t == "Str" and primeiro.text:gsub("%.$", "")
  local conteudo = pandoc.Inlines(h.content)
  if num and num:match("^%d+$") then
    h.level = 1
    vistos[num] = true
  elseif num and num:match("^%d+%.%d+$") then
    h.level = 2
    local cap = num:match("^%d+")
    if not vistos[cap] then
      error("section " .. num .. " has no chapter heading above it: add \"## " .. cap
        .. ". <title>\" to the .md before it")
    end
  else
    num = nil
  end
  if num then
    conteudo:remove(1)
    if conteudo[1] and conteudo[1].t == "Space" then conteudo:remove(1) end
  end
  local ultimo = conteudo[#conteudo]
  if ultimo and ultimo.t == "Str" then
    ultimo.text = ultimo.text:gsub(":$", "")
  end
  h.content = conteudo
  saida:insert(h)
end

-- "## Glossary" (after the handwritten references) holds a definition list whose
-- terms carry a label: [vTable]{#gls:vtable}. It is printed after the reference
-- list, sorted alphabetically; @gls:vtable in the text links to the entry.
local GLOSSARIO = {}

local function latex_blocos(blocos)
  local saida = pandoc.write(pandoc.Pandoc(blocos), "latex")
  return (saida:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function glossario(corpo)
  local itens = {}
  for _, b in ipairs(corpo) do
    if b.t ~= "DefinitionList" then
      error("## Glossary must contain only a definition list (term, then ': definition')")
    end
    for _, item in ipairs(b.content) do
      local termo, defs = item[1], item[2]
      local id, conteudo
      for _, x in ipairs(termo) do
        if x.t == "Span" and x.identifier:match("^gls:") then id, conteudo = x.identifier, x.content end
      end
      if not id then
        error("glossary term '" .. pandoc.utils.stringify(termo) .. "' has no {#gls:...} label")
      end
      if GLOSSARIO[id] then error("glossary label #" .. id .. " is used twice") end
      GLOSSARIO[id] = conteudo
      local def = pandoc.Blocks({})
      for _, d in ipairs(defs) do def:extend(d) end
      itens[#itens + 1] = { chave = pandoc.utils.stringify(conteudo):lower(), id = id,
                            termo = conteudo, def = def }
    end
  end
  table.sort(itens, function(a, b) return a.chave < b.chave end)
  local linhas = { "\\begin{description}" }
  for _, it in ipairs(itens) do
    linhas[#linhas + 1] = "\\item[{\\phantomsection\\label{" .. it.id .. "}" .. latex(it.termo)
      .. ":}] " .. latex_blocos(it.def)
  end
  linhas[#linhas + 1] = "\\end{description}"
  return {
    pandoc.Header(1, "Glossary", { class = "unnumbered" }),
    pandoc.RawBlock("latex", table.concat(linhas, "\n")),
  }
end

local function ler_metadados()
  local arquivo = assert(io.open("abnt/metadados.yaml"))
  local yaml = arquivo:read("a")
  arquivo:close()
  return pandoc.read("---\n" .. yaml .. "\n---\n", "markdown").meta
end

function Reader(entrada, opcoes)
  local texto = converter_citacoes(tostring(entrada))
  local doc = pandoc.read(texto,
    "markdown+lists_without_preceding_blankline", opcoes)
  local meta = ler_metadados()

  local blocos = doc.blocks
  local saida = pandoc.Blocks({})
  local vistos = {}
  local i = 1

  -- Everything before the first numbered heading is front matter. "Resumo" and
  -- "Abstract" go to the template's own pages; the draft banner, title and
  -- author lines are dropped, since the template builds those from metadata.
  local FRENTE = { Resumo = "resumo", Abstract = "abstract" }
  local secoes, atual = {}, nil
  while blocos[i] and not (blocos[i].t == "Header"
      and pandoc.utils.stringify(blocos[i]):match("^%d")) do
    local b = blocos[i]
    if b.t == "Header" then
      atual = FRENTE[pandoc.utils.stringify(b)]
      if atual then secoes[atual] = pandoc.Blocks({}) end
    elseif atual then
      secoes[atual]:insert(b)
    end
    i = i + 1
  end
  for chave_meta in pairs(FRENTE) do
    local campo = FRENTE[chave_meta]
    if not secoes[campo] then error("missing \"## " .. chave_meta .. "\" section in the .md") end
    doc.meta[campo] = pandoc.MetaBlocks(secoes[campo])
  end

  while blocos[i] do
    local b = blocos[i]
    if b.t == "Header" and pandoc.utils.stringify(b) == "References" then
      saida:insert(pandoc.RawBlock("latex", "\\postextual"))
      saida:insert(pandoc.Header(1, "References", { class = "unnumbered" }))
      saida:insert(pandoc.RawBlock("latex", "\\begingroup\\SingleSpacing\\raggedright"))
      saida:insert(pandoc.Div({}, { id = "refs" }))
      saida:insert(pandoc.RawBlock("latex", "\\endgroup"))
      -- The handwritten list is replaced by the citeproc one; only the glossary
      -- after it is kept.
      local g = i + 1
      while blocos[g] and not (blocos[g].t == "Header"
          and pandoc.utils.stringify(blocos[g]) == "Glossary") do
        g = g + 1
      end
      if blocos[g] then
        local corpo = pandoc.Blocks({})
        g = g + 1
        while blocos[g] and blocos[g].t ~= "Header" do
          corpo:insert(blocos[g])
          g = g + 1
        end
        saida:extend(glossario(corpo))
      end
      break
    elseif b.t == "Header" then
      titulo(b, vistos, meta, saida)
    elseif b.t == "Figure" then
      local inlines = legenda(blocos[i + 1], "Figure")
      if inlines then i = i + 1 end
      saida:insert(figura(b, inlines or pandoc.Inlines({}), meta))
    elseif b.t == "Para" and legenda_colada(b) then
      saida:insert(figura(b, legenda_colada(b), meta))
    elseif b.t == "Table" then
      local inlines = legenda(blocos[i + 1], "Matrix") or legenda(blocos[i + 1], "Table")
      if inlines then i = i + 1 end
      saida:extend(tabela(b, inlines))
    elseif b.t == "CodeBlock" then
      local inlines = legenda(blocos[i + 1], "Figure")
      if inlines then i = i + 1 end
      saida:insert(codigo(b, inlines))
    else
      saida:insert(b)
    end
    i = i + 1
  end

  -- @fig:name in the text => "Figure N", linked to the figure. Pandoc reads it as a
  -- citation, so it is replaced here, before citeproc sees it.
  saida = saida:walk({ Cite = function(c)
    local id = c.citations[1].id
    if id:match("^gls:") then
      if #c.citations > 1 then error("@" .. id .. ": cite one glossary term at a time") end
      if not GLOSSARIO[id] then error("@" .. id .. " points to no glossary term with {#" .. id .. "}") end
      return pandoc.RawInline("latex", "\\hyperref[" .. id .. "]{" .. latex(GLOSSARIO[id]) .. "}")
    end
    if not id:match("^fig:") then return nil end
    if #c.citations > 1 then error("@" .. id .. ": cite one figure at a time") end
    if not ROTULOS[id] then error("@" .. id .. " points to no figure with {#" .. id .. "}") end
    return pandoc.RawInline("latex", "\\hyperref[" .. id .. "]{\\figurename~\\ref*{" .. id .. "}}")
  end, Link = function(l)
    -- [text](#gls:name) links to a glossary term with different wording (plural, case).
    local id = l.target:match("^#(gls:.*)")
    if id and not GLOSSARIO[id] then error("link to #" .. id .. ": no glossary term has that label") end
  end })

  doc.blocks = saida
  return doc
end
