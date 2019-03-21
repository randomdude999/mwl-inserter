This is a MWL inserter for SMW.

Desired end result: ability to insert a (set of) MWL along with its sublevels and relevant GFX files into the ROM with minimal human interaction.

Plan:
1. Read all input MWLs, figure out which levels to insert them into and store that as a mapping.
2. Figure out all used secondary entrances of the input MWLs, find what numbers to use for them and store that mapping for later.
3. Figure out which GFX files said levels use and locate unused GFX slots for them. Store that as a mapping for later too.
4. Figure out which custom Map16 tiles the level uses, find what those map16 tiles were supposed to be, and keep track of them for later.
5. Actually insert the levels!
    5a. Layer 2 BG data is stored uncompressed in the MWL, but RLE1'd in the ROM - so, we need to compress it.
    5b. Layer 1 data will need to be modified a bit - mainly, screen exit objects need to be remapped using the mappings from points 1 and 2, and direct map16 objects need to be remapped using the mapping from point 4.
    5c. Level properties can mostly be copied over, but GFX file numbers of course need to be remapped.
    5d. Most other sections can be directly copied over.
6. Insert all the GFX files into their respective slots.

The only thing that Asar is really useful for here is (a) handling SNES->PC address conversion (pretty easy), and (b) finding freespace. Ideally I'd like to get rid of the Asar dependency eventually. The hardest part would probably be porting the freespace finding algorithm to a separate module.

TODOs:
* Currently, only L1/L2/Sprite/secondary entrances are handled at all. We will need to handle palette/exanim/bypass info too.
* Handle Map16 remapping for custom map16 tiles.
* Insert GFX files into the correct place.
* Remove Asar dependency.
* Make the input a bit more configurable - allow, say, a list of MWL files, or a directory with MWL files, or maybe even a directory of directories (allows keeping GFX files near levels neatly)

Potential handling of other types of resources later:
* UberASM - allowing a single levelASM file per level would be fine, and would just need exporting the mapping of mwl level number -> ROM level number.
* Music - Would need to modify the L1 data since the music replacer is there, but that's not much of a problem since we already modify L1
* Sprites - Could use PIXI's per-level sprites for this. This would need editing sprite data to make sure all custom sprites are per-level ones.
* Blocks - Would need keeping track of which map16 tiles are used where and editing L1 data later, but this probably wouldn't be too difficult either.
